#include "video_stream.h"

#include <QTimer>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QVideoSink>
#include <QDebug>
#include <QStringList>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/cpu.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libavutil/mathematics.h>
#include <libswscale/swscale.h>
}

struct VideoStreamBackend
{
    std::thread worker;
    std::atomic<bool> stopRequested{false};
    std::atomic<bool> running{false};

    std::atomic<int> sinkCount{0};

    std::shared_ptr<std::atomic<int>> framesInFlight{std::make_shared<std::atomic<int>>(0)};
    std::shared_ptr<std::atomic<quint64>> epoch{std::make_shared<std::atomic<quint64>>(0)};
};

namespace {

constexpr int kMaxRetries = 5;
constexpr int kRetryDelayMs = 2000;
constexpr int kMaxFramesInFlight = 2;
constexpr qint64 kMaxLagMs = 1000;
constexpr qint64 kResyncTimeoutMs = 3000;
constexpr qint64 kResyncClusterMs = 5000;
constexpr int kResyncBurstToEscalate = 3;
constexpr int kMaxDecodeThreads = 4;

constexpr const char* kProbeSize = "65536";
constexpr const char* kAnalyzeDurationUs = "500000";

constexpr const char* kSocketTimeoutUs = "5000000";

QVideoFrameFormat::PixelFormat toQtPixelFormat(AVPixelFormat format)
{
    switch (format) {
    case AV_PIX_FMT_YUV420P:  return QVideoFrameFormat::Format_YUV420P;
    case AV_PIX_FMT_YUVJ420P: return QVideoFrameFormat::Format_YUV420P;
    case AV_PIX_FMT_NV12:     return QVideoFrameFormat::Format_NV12;
    case AV_PIX_FMT_RGBA:     return QVideoFrameFormat::Format_RGBA8888;
    case AV_PIX_FMT_BGRA:     return QVideoFrameFormat::Format_BGRA8888;
    default:                  return QVideoFrameFormat::Format_Invalid;
    }
}

QString threadTypeName(int type)
{
    QStringList parts;
    if (type & FF_THREAD_FRAME) {
        parts << QStringLiteral("frame");
    }
    if (type & FF_THREAD_SLICE) {
        parts << QStringLiteral("slice");
    }
    return parts.isEmpty() ? QStringLiteral("single") : parts.join(QLatin1Char('+'));
}

QString avError(int code)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, buf, sizeof(buf));
    return QString::fromUtf8(buf);
}

void installFfmpegLogBridge()
{
    static bool installed = false;
    if (installed) {
        return;
    }
    installed = true;

    av_log_set_level(AV_LOG_ERROR);
    av_log_set_callback([](void* ptr, int level, const char* fmt, va_list vl) {
        if (level > av_log_get_level()) {
            return;
        }
        char line[1024] = {};
        int prefix = 1;
        av_log_format_line2(ptr, level, fmt, vl, line, sizeof(line), &prefix);
        const QString text = QString::fromUtf8(line).trimmed();
        if (!text.isEmpty()) {
            qWarning().noquote() << "VIDEO:" << text;
        }
    });
}

void copyPlanes(QVideoFrame& frame, const AVFrame* src)
{
    const int planes = qMin(frame.planeCount(), AV_NUM_DATA_POINTERS);
    for (int plane = 0; plane < planes; ++plane) {
        const uchar* from = src->data[plane];
        uchar* to = frame.bits(plane);
        const int srcStride = src->linesize[plane];
        const int dstStride = frame.bytesPerLine(plane);
        if (!from || !to || srcStride <= 0 || dstStride <= 0) {
            continue;
        }
        const int rowBytes = qMin(srcStride, dstStride);
        const int rows = frame.mappedBytes(plane) / dstStride;
        for (int row = 0; row < rows; ++row) {
            memcpy(to + row * dstStride, from + row * srcStride, rowBytes);
        }
    }
}

} // namespace

VideoStream::VideoStream(QObject* parent)
    : QObject(parent)
    , backend_(new VideoStreamBackend)
{
}

VideoStream::~VideoStream()
{
    closeStream();
    delete backend_;
}

bool VideoStream::backendAvailable() const
{
    return true;
}

QString VideoStream::backendName() const
{
    return QStringLiteral("ffmpeg");
}

bool VideoStream::isActive() const
{
    return backend_->running.load();
}

void VideoStream::addSink(QObject* sink)
{
    QVideoSink* next = qobject_cast<QVideoSink*>(sink);
    if (!next || videoSinks_.contains(next)) {
        return;
    }

    videoSinks_.append(next);
    connect(next, &QObject::destroyed, this, [this]() { syncSinkCount(); });
    syncSinkCount();
}

void VideoStream::removeSink(QObject* sink)
{
    QVideoSink* target = qobject_cast<QVideoSink*>(sink);
    if (!target) {
        return;
    }

    if (videoSinks_.removeAll(QPointer<QVideoSink>(target)) > 0) {
        target->disconnect(this);
        target->setVideoFrame(QVideoFrame());
    }
    syncSinkCount();
}

void VideoStream::syncSinkCount()
{
    videoSinks_.removeIf([](const QPointer<QVideoSink>& sink) { return sink.isNull(); });

    const int count = videoSinks_.size();
    if (backend_->sinkCount.exchange(count) != count) {
        emit sinkCountChanged();
    }
}

void VideoStream::deliverFrame(const QVideoFrame& frame)
{
    for (const QPointer<QVideoSink>& sink : videoSinks_) {
        if (!sink.isNull()) {
            sink->setVideoFrame(frame);
        }
    }
}

void VideoStream::clearSinks()
{
    for (const QPointer<QVideoSink>& sink : videoSinks_) {
        if (!sink.isNull()) {
            sink->setVideoFrame(QVideoFrame());
        }
    }
}

void VideoStream::setStatusText(const QString& text)
{
    if (statusText_ == text) {
        return;
    }
    statusText_ = text;
    emit statusTextChanged();
}

void VideoStream::start(const QString& url)
{
    const QString trimmed = url.trimmed();
    stop();

    if (trimmed.isEmpty()) {
        return;
    }

    url_ = trimmed;
    emit urlChanged();
    retryCount_ = 0;

    setStatusText(tr("Connecting..."));
    openStream();
}

void VideoStream::stop()
{
    const bool wasActive = isActive();
    const bool hadUrl = !url_.isEmpty();

    closeStream();
    retryCount_ = 0;

    if (hadUrl) {
        url_.clear();
        emit urlChanged();
    }
    if (wasActive) {
        emit activeChanged();
    }
    setStatusText(QString());
}

void VideoStream::openStream()
{
    installFfmpegLogBridge();
    closeStream();

    backend_->stopRequested.store(false);
    backend_->framesInFlight->store(0);
    backend_->running.store(true);

    const QByteArray location = url_.toUtf8();
    auto epoch = backend_->epoch;
    const quint64 generation = epoch->load();

    backend_->worker = std::thread([this, location, epoch, generation]() {
        const auto post = [this, epoch, generation](auto&& action) {
            QMetaObject::invokeMethod(this, [epoch, generation, action]() {
                if (epoch->load() == generation) {
                    action();
                }
            }, Qt::QueuedConnection);
        };
        AVFormatContext* fmt = avformat_alloc_context();
        if (!fmt) {
            post([this]() { handlePipelineError(tr("out of memory")); });
            return;
        }

        fmt->interrupt_callback.opaque = backend_;
        fmt->interrupt_callback.callback = [](void* opaque) -> int {
            return static_cast<VideoStreamBackend*>(opaque)->stopRequested.load() ? 1 : 0;
        };

        AVDictionary* opts = nullptr;
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
        av_dict_set(&opts, "timeout", kSocketTimeoutUs, 0);
        av_dict_set(&opts, "max_delay", "200000", 0);
        av_dict_set(&opts, "fflags", "nobuffer", 0);
        av_dict_set(&opts, "probesize", kProbeSize, 0);
        av_dict_set(&opts, "analyzeduration", kAnalyzeDurationUs, 0);

        int rc = avformat_open_input(&fmt, location.constData(), nullptr, &opts);
        av_dict_free(&opts);
        if (rc < 0) {
            if (!backend_->stopRequested.load()) {
                const QString text = avError(rc);
                post([this, text]() { handlePipelineError(text); });
            }
            return;
        }

        rc = avformat_find_stream_info(fmt, nullptr);
        if (rc < 0) {
            avformat_close_input(&fmt);
            if (!backend_->stopRequested.load()) {
                const QString text = avError(rc);
                post([this, text]() { handlePipelineError(text); });
            }
            return;
        }

        const int videoIndex = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (videoIndex < 0) {
            avformat_close_input(&fmt);
            post([this]() { handlePipelineError(tr("no video stream")); });
            return;
        }

        AVStream* stream = fmt->streams[videoIndex];
        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        const int decodeThreads = std::min(av_cpu_count(), kMaxDecodeThreads);

        const auto openDecoder = [codec, stream, decodeThreads](int threadType) -> AVCodecContext* {
            AVCodecContext* ctx = codec ? avcodec_alloc_context3(codec) : nullptr;
            if (!ctx) {
                return nullptr;
            }
            if (avcodec_parameters_to_context(ctx, stream->codecpar) < 0) {
                avcodec_free_context(&ctx);
                return nullptr;
            }
            ctx->thread_count = decodeThreads;
            ctx->thread_type = threadType;
            if (avcodec_open2(ctx, codec, nullptr) < 0) {
                avcodec_free_context(&ctx);
                return nullptr;
            }
            return ctx;
        };

        AVCodecContext* dec = openDecoder(FF_THREAD_SLICE);
        if (!dec) {
            avformat_close_input(&fmt);
            post([this]() { handlePipelineError(tr("no decoder for this stream")); });
            return;
        }

        qInfo().noquote() << QStringLiteral("VIDEO: decoder %1, cores %2, threads %3 (%4)")
                                 .arg(QString::fromUtf8(codec->name))
                                 .arg(av_cpu_count())
                                 .arg(dec->thread_count > 0 ? QString::number(dec->thread_count)
                                                            : QStringLiteral("auto"))
                                 .arg(threadTypeName(dec->active_thread_type));

        AVPacket* packet = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        AVFrame* converted = nullptr;
        SwsContext* scaler = nullptr;
        int postedWidth = 0;
        int postedHeight = 0;

        const AVRational streamTimeBase = stream->time_base;
        const bool lagGuardEnabled = streamTimeBase.num > 0 && streamTimeBase.den > 0;
        if (!lagGuardEnabled) {
            qWarning().noquote()
                << QStringLiteral("VIDEO: time base %1/%2 unusable, latency guard off")
                       .arg(streamTimeBase.num)
                       .arg(streamTimeBase.den);
        }
        std::chrono::steady_clock::time_point wallBase{};
        qint64 streamBaseMs = 0;
        bool lagBaseValid = false;
        bool awaitingKeyframe = false;
        std::chrono::steady_clock::time_point resyncStart{};
        std::chrono::steady_clock::time_point lastResync{};
        bool haveLastResync = false;
        int resyncBurst = 0;
        bool frameThreading = false;
        bool revertToLowLatency = false;

        while (!backend_->stopRequested.load()) {
            rc = av_read_frame(fmt, packet);
            if (rc < 0) {
                if (rc == AVERROR(EAGAIN)) {
                    continue;
                }
                break;
            }
            if (packet->stream_index != videoIndex) {
                av_packet_unref(packet);
                continue;
            }

            if (revertToLowLatency) {
                revertToLowLatency = false;
                AVCodecContext* leaner = openDecoder(FF_THREAD_SLICE);
                if (leaner) {
                    avcodec_free_context(&dec);
                    dec = leaner;
                    frameThreading = false;
                    resyncBurst = 0;
                    haveLastResync = false;
                    lagBaseValid = false;
                    awaitingKeyframe = true;
                    resyncStart = std::chrono::steady_clock::now();
                    qInfo().noquote()
                        << QStringLiteral("VIDEO: source now %1x%2, back to slice threading")
                               .arg(postedWidth)
                               .arg(postedHeight);
                }
            }

            if (awaitingKeyframe) {
                const qint64 waitedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                            std::chrono::steady_clock::now() - resyncStart).count();
                const bool giveUp = waitedMs > kResyncTimeoutMs;
                if (!(packet->flags & AV_PKT_FLAG_KEY) && !giveUp) {
                    av_packet_unref(packet);
                    continue;
                }
                if (giveUp) {
                    qWarning().noquote()
                        << QStringLiteral("VIDEO: no keyframe in %1 ms, resuming without one")
                               .arg(waitedMs);
                }
                avcodec_flush_buffers(dec);
                awaitingKeyframe = false;
                lagBaseValid = false;
            }

            if (lagGuardEnabled && packet->pts != AV_NOPTS_VALUE) {
                const qint64 ptsMs =
                    av_rescale_q(packet->pts, streamTimeBase, AVRational{1, 1000});
                const auto now = std::chrono::steady_clock::now();
                if (!lagBaseValid) {
                    wallBase = now;
                    streamBaseMs = ptsMs;
                    lagBaseValid = true;
                }
                else {
                    const qint64 wallMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                              now - wallBase).count();
                    const qint64 lagMs = wallMs - (ptsMs - streamBaseMs);
                    if (lagMs > kMaxLagMs) {
                        qInfo().noquote()
                            << QStringLiteral("VIDEO: behind by %1 ms, resync to keyframe")
                                   .arg(lagMs);
                        awaitingKeyframe = true;
                        resyncStart = now;

                        const qint64 sinceLastMs = haveLastResync
                            ? std::chrono::duration_cast<std::chrono::milliseconds>(
                                  now - lastResync).count()
                            : -1;
                        resyncBurst = (sinceLastMs >= 0 && sinceLastMs < kResyncClusterMs)
                                          ? resyncBurst + 1
                                          : 1;
                        haveLastResync = true;
                        lastResync = now;

                        if (!frameThreading && resyncBurst >= kResyncBurstToEscalate) {
                            AVCodecContext* faster =
                                openDecoder(FF_THREAD_FRAME | FF_THREAD_SLICE);
                            if (faster) {
                                avcodec_free_context(&dec);
                                dec = faster;
                                frameThreading = true;
                                qInfo().noquote()
                                    << QStringLiteral("VIDEO: %1 resyncs, enabling frame "
                                                      "threading, %2 threads (%3)")
                                           .arg(resyncBurst)
                                           .arg(dec->thread_count)
                                           .arg(threadTypeName(dec->active_thread_type));
                            }
                        }

                        av_packet_unref(packet);
                        continue;
                    }
                    if (lagMs < -kMaxLagMs) {
                        wallBase = now;
                        streamBaseMs = ptsMs;
                    }
                }
            }

            rc = avcodec_send_packet(dec, packet);
            av_packet_unref(packet);
            if (rc < 0 && rc != AVERROR(EAGAIN)) {
                continue;
            }

            while (avcodec_receive_frame(dec, frame) == 0) {
                const AVFrame* out = frame;
                QVideoFrameFormat::PixelFormat qtFormat =
                    toQtPixelFormat(static_cast<AVPixelFormat>(frame->format));

                if (qtFormat == QVideoFrameFormat::Format_Invalid) {
                    if (!scaler || !converted
                            || converted->width != frame->width
                            || converted->height != frame->height) {
                        sws_freeContext(scaler);
                        scaler = nullptr;
                        if (converted) {
                            av_frame_free(&converted);
                        }
                        scaler = sws_getContext(frame->width, frame->height,
                                                static_cast<AVPixelFormat>(frame->format),
                                                frame->width, frame->height, AV_PIX_FMT_YUV420P,
                                                SWS_BILINEAR, nullptr, nullptr, nullptr);
                        converted = av_frame_alloc();
                        if (converted) {
                            converted->format = AV_PIX_FMT_YUV420P;
                            converted->width = frame->width;
                            converted->height = frame->height;
                            if (av_frame_get_buffer(converted, 32) < 0) {
                                av_frame_free(&converted);
                            }
                        }
                    }
                    if (!scaler || !converted) {
                        continue;
                    }
                    sws_scale(scaler, frame->data, frame->linesize, 0, frame->height,
                              converted->data, converted->linesize);
                    out = converted;
                    qtFormat = QVideoFrameFormat::Format_YUV420P;
                }

                if (out->width != postedWidth || out->height != postedHeight) {
                    if (postedWidth > 0 && frameThreading) {
                        revertToLowLatency = true;
                    }
                    postedWidth = out->width;
                    postedHeight = out->height;
                    const int w = postedWidth;
                    const int h = postedHeight;
                    post([this, w, h]() { applySourceSize(w, h); });
                }

                if (backend_->sinkCount.load() == 0
                        || backend_->framesInFlight->load() >= kMaxFramesInFlight) {
                    continue;
                }

                QVideoFrame videoFrame(QVideoFrameFormat(QSize(out->width, out->height), qtFormat));
                if (!videoFrame.map(QVideoFrame::WriteOnly)) {
                    continue;
                }
                copyPlanes(videoFrame, out);
                videoFrame.unmap();

                backend_->framesInFlight->fetch_add(1);
                auto inFlight = backend_->framesInFlight;
                QMetaObject::invokeMethod(this, [this, videoFrame, inFlight, epoch, generation]() {
                    if (epoch->load() == generation) {
                        deliverFrame(videoFrame);
                    }
                    inFlight->fetch_sub(1);
                }, Qt::QueuedConnection);
            }
        }

        const bool aborted = backend_->stopRequested.load();

        if (scaler) {
            sws_freeContext(scaler);
        }
        if (converted) {
            av_frame_free(&converted);
        }
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&dec);
        avformat_close_input(&fmt);

        if (!aborted) {
            post([this]() { handlePipelineError(tr("Stream ended")); });
        }
    });

    emit activeChanged();
}

void VideoStream::closeStream()
{
    backend_->epoch->fetch_add(1);
    backend_->stopRequested.store(true);
    if (backend_->worker.joinable()) {
        backend_->worker.join();
    }
    backend_->running.store(false);
    backend_->stopRequested.store(false);

    clearSinks();

    if (sourceWidth_ != 0 || sourceHeight_ != 0) {
        sourceWidth_ = 0;
        sourceHeight_ = 0;
        emit sourceSizeChanged();
        emit hasFrameChanged();
    }
}

void VideoStream::applySourceSize(int width, int height)
{
    if (sourceWidth_ == width && sourceHeight_ == height) {
        return;
    }
    const bool had = hasFrame();
    sourceWidth_ = width;
    sourceHeight_ = height;
    emit sourceSizeChanged();
    if (had != hasFrame()) {
        emit hasFrameChanged();
    }
    if (hasFrame()) {
        setStatusText(QString());
        retryCount_ = 0;
    }
}

void VideoStream::handlePipelineError(const QString& text)
{
    qWarning().noquote() << "VIDEO: stream error:" << text;
    closeStream();
    emit activeChanged();

    if (url_.isEmpty()) {
        setStatusText(QString());
        return;
    }
    if (retryCount_ >= kMaxRetries) {
        setStatusText(QString());
        emit retriesExhausted();
        return;
    }
    ++retryCount_;
    QTimer::singleShot(kRetryDelayMs, this, &VideoStream::retryStart);
}

void VideoStream::retryStart()
{
    if (url_.isEmpty() || isActive()) {
        return;
    }
    setStatusText(tr("Reconnecting (%1/%2)...").arg(retryCount_).arg(kMaxRetries));
    openStream();
}
