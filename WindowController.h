#pragma once

#include <QObject>
#include <QWindow>


class WindowController : public QObject {
    Q_OBJECT

public:
    explicit WindowController(QWindow* window, QObject* parent = nullptr);
    void setWindow(QWindow* window);

public slots:
    void toggleFullScreen();

private:
    QWindow* window_;
    bool isFullScreen_;
};
