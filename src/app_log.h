#ifndef APP_LOG_H
#define APP_LOG_H

#include <QFile>
#include <QMutex>
#include <QString>
#include <QtLogging>

class QMessageLogContext;

class AppLog
{
public:
    static AppLog& instance();

    bool start(const QString& dirPath, const QString& baseName, qint64 maxBytes, int maxFiles);
    bool relocate(const QString& dirPath);
    void stop();

    void write(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    void writeRaw(QtMsgType type, const QString& category, const QString& msg);

    QString directory() const;
    QString currentFilePath() const;
    bool isActive() const;

    static QString defaultDirectory();
    static QString fallbackDirectory();

private:
    AppLog() = default;
    AppLog(const AppLog&) = delete;
    AppLog& operator=(const AppLog&) = delete;

    QString filePathIn(const QString& dir, int index) const;
    QString filePathUnlocked(int index) const;
    void appendUnlocked(QtMsgType type, const QString& category, const QString& msg);
    void rotateUnlocked();

    mutable QMutex mutex_;
    QFile file_;
    QString dir_;
    QString base_;
    qint64 maxBytes_ = 0;
    qint64 written_ = 0;
    int maxFiles_ = 0;
    bool active_ = false;
};

#endif // APP_LOG_H
