#include "notifications.h"
#include "core.h"

extern Core core;

Notifications::Notifications(QObject* parent)
    : QObject(parent)
{
}

void Notifications::info(const QString& text, const QString& actionPath)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    core.consoleNotification(trimmed, false);
    emit messageRequested(0, trimmed, QString(), actionPath);
}

void Notifications::warning(const QString& text, const QString& tag, const QString& actionPath)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    core.consoleNotification(trimmed, true);
    emit messageRequested(1, trimmed, tag, actionPath);
}

void Notifications::progress(const QString& text, const QString& tag, int percent)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || tag.isEmpty()) {
        return;
    }
    emit progressRequested(trimmed, tag, percent < 0 ? -1 : qMin(percent, 100));
}

void Notifications::dismiss(const QString& tag)
{
    if (tag.isEmpty()) {
        return;
    }
    emit dismissRequested(tag);
}
