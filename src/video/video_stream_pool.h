#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantList>

class VideoStream;

class VideoStreamPool : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList streams READ streams NOTIFY streamsChanged)
    Q_PROPERTY(int count READ count NOTIFY streamsChanged)

public:
    explicit VideoStreamPool(QObject* parent = nullptr);
    ~VideoStreamPool() override;

    void setSourceModel(QAbstractItemModel* model);

    QVariantList streams() const { return descriptors_; }
    int count() const { return descriptors_.size(); }

    Q_INVOKABLE QObject* streamFor(const QString& uuid) const;
    Q_INVOKABLE bool isStreamOpen(const QString& uuid) const;
    Q_INVOKABLE QString labelFor(const QString& uuid) const;
    Q_INVOKABLE QString firstOpenUuid() const;

signals:
    void streamsChanged();

private slots:
    void refresh();

private:
    static QString buildUrl(const QString& address);

    void rebuild();

    bool refreshing_ = false;
    bool refreshPending_ = false;

    QPointer<QAbstractItemModel> model_;
    QHash<QString, VideoStream*> streams_;
    QHash<QString, QString> failedUrls_;
    QVariantList descriptors_;
};
