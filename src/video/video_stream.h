#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

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
    Q_PROPERTY(QObject* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
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
    QObject* videoSink() const;
    void setVideoSink(QObject* sink);
    bool backendAvailable() const;
    QString backendName() const;

    Q_INVOKABLE void start(const QString& url);
    Q_INVOKABLE void stop();

signals:
    void urlChanged();
    void activeChanged();
    void hasFrameChanged();
    void sourceSizeChanged();
    void statusTextChanged();
    void videoSinkChanged();
    void retriesExhausted();

private slots:
    void applySourceSize(int width, int height);
    void handlePipelineError(const QString& text);
    void retryStart();

private:
    void openStream();
    void closeStream();
    void setStatusText(const QString& text);

    VideoStreamBackend* backend_ = nullptr;

    QPointer<QVideoSink> videoSink_;
    QString url_;
    QString statusText_;
    int sourceWidth_ = 0;
    int sourceHeight_ = 0;
    int retryCount_ = 0;
};
