#pragma once

#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

class QVideoFrame;

class QVideoSink;

struct VideoStreamBackend;

class VideoStream : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString url READ url NOTIFY urlChanged)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY hasFrameChanged)
    Q_PROPERTY(int sourceWidth READ sourceWidth NOTIFY sourceSizeChanged)
    Q_PROPERTY(int sourceHeight READ sourceHeight NOTIFY sourceSizeChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(int sinkCount READ sinkCount NOTIFY sinkCountChanged)
    Q_PROPERTY(bool backendAvailable READ backendAvailable CONSTANT)
    Q_PROPERTY(QString backendName READ backendName CONSTANT)

public:
    explicit VideoStream(QObject* parent = nullptr);
    ~VideoStream() override;

    QString url() const { return url_; }
    bool isActive() const;
    bool hasFrame() const { return sourceWidth_ > 0 && sourceHeight_ > 0; }
    int sourceWidth() const { return sourceWidth_; }
    int sourceHeight() const { return sourceHeight_; }
    QString statusText() const { return statusText_; }
    int sinkCount() const { return videoSinks_.size(); }
    bool backendAvailable() const;
    QString backendName() const;

    Q_INVOKABLE void start(const QString& url);
    Q_INVOKABLE void stop();
    void stopWithFailure();
    Q_INVOKABLE void addSink(QObject* sink);
    Q_INVOKABLE void removeSink(QObject* sink);

signals:
    void urlChanged();
    void activeChanged();
    void hasFrameChanged();
    void sourceSizeChanged();
    void statusTextChanged();
    void sinkCountChanged();
    void retriesExhausted();

private slots:
    void applySourceSize(int width, int height);
    void handlePipelineError(const QString& text);
    void retryStart();

private:
    void openStream();
    void closeStream();
    void doStop(const QString& finalStatus);
    void setStatusText(const QString& text);
    void deliverFrame(const QVideoFrame& frame);
    void clearSinks();
    void syncSinkCount();

    VideoStreamBackend* backend_ = nullptr;

    QList<QPointer<QVideoSink>> videoSinks_;
    QString url_;
    QString statusText_;
    int sourceWidth_ = 0;
    int sourceHeight_ = 0;
    int retryCount_ = 0;
};
