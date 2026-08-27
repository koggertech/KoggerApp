#include "video_stream.h"

#include <QTimer>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QVideoSink>
#include <QDebug>

#include <atomic>
#include <memory>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
}

struct VideoStreamBackend
{
    std::thread worker;
    std::atomic<bool> stopRequested{false};
    std::atomic<bool> running{false};

    std::atomic<QVideoSink*> sink{nullptr};

    std::shared_ptr<std::atomic<int>> framesInFlight{std::make_shared<std::atomic<int>>(0)};
    std::shared_ptr<std::atomic<quint64>> epoch{std::make_shared<std::atomic<quint64>>(0)};
};

namespace {

constexpr int kMaxRetries = 5;
constexpr int kRetryDelayMs = 2000;
constexpr int kMaxFramesInFlight = 2;

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

QObject* VideoStream::videoSink() const
{
    return videoSink_;
}

void VideoStream::setVideoSink(QObject* sink)
{
    QVideoSink* next = qobject_cast<QVideoSink*>(sink);
    if (videoSink_ == next) {
        return;
    }
    videoSink_ = next;
    backend_->sink.store(next);
    if (next) {
        VideoStreamBackend* backend = backend_;
        connect(next, &QObject::destroyed, this, [backend, next]() {
            QVideoSink* expected = next;
            backend->sink.compare_exchange_strong(expected, nullptr);
        });
    }
    emit videoSinkChanged();
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
        AVCodecContext* dec = codec ? avcodec_alloc_context3(codec) : nullptr;
        if (!dec || avcodec_parameters_to_context(dec, stream->codecpar) < 0
                 || avcodec_open2(dec, codec, nullptr) < 0) {
            if (dec) {
                avcodec_free_context(&dec);
            }
            avformat_close_input(&fmt);
            post([this]() { handlePipelineError(tr("no decoder for this stream")); });
            return;
        }

        qInfo().noquote() << QStringLiteral("VIDEO: decoder %1, threads %2")
                                 .arg(QString::fromUtf8(codec ? codec->name : "?"))
                                 .arg(dec->thread_count);

        AVPacket* packet = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        AVFrame* converted = nullptr;
        SwsContext* scaler = nullptr;
        int postedWidth = 0;
        int postedHeight = 0;

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
                    postedWidth = out->width;
                    postedHeight = out->height;
                    const int w = postedWidth;
                    const int h = postedHeight;
                    post([this, w, h]() { applySourceSize(w, h); });
                }

                QVideoSink* sink = backend_->sink.load();
                if (!sink || backend_->framesInFlight->load() >= kMaxFramesInFlight) {
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
                QMetaObject::invokeMethod(sink, [sink, videoFrame, inFlight, epoch, generation]() {
                    if (epoch->load() == generation) {
                        sink->setVideoFrame(videoFrame);
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

    if (!videoSink_.isNull()) {
        videoSink_->setVideoFrame(QVideoFrame());
    }

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
