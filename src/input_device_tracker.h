#pragma once

#include <QObject>
#include <QColor>

class QEvent;

class InputDeviceTracker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentMode READ currentMode NOTIFY currentModeChanged)
    Q_PROPERTY(bool touchMode READ touchMode NOTIFY currentModeChanged)
    Q_PROPERTY(QString displayLabel READ displayLabel NOTIFY currentModeChanged)
    Q_PROPERTY(QColor displayColor READ displayColor NOTIFY currentModeChanged)

public:
    explicit InputDeviceTracker(QObject* parent = nullptr);

    QString currentMode() const;
    bool touchMode() const;
    QString displayLabel() const;
    QColor displayColor() const;

    Q_INVOKABLE void markMouseKeyboardInput();
    Q_INVOKABLE void markKeyboardInput();
    Q_INVOKABLE void markTouchInput();

signals:
    void currentModeChanged();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    enum class Mode {
        MouseKeyboard,
        TouchScreen,
    };

    void setMode(Mode mode);
    qint64 nowMs() const;
    bool shouldSuppressMouseInput() const;

    Mode m_mode = Mode::MouseKeyboard;
    qint64 m_suppressMouseUntilMs = 0;
};
