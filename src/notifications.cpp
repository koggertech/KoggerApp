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
    core.consoleInfo(trimmed);
    emit messageRequested(0, trimmed, QString(), actionPath);
}

void Notifications::warning(const QString& text, const QString& tag, const QString& actionPath)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    core.consoleWarning(trimmed);
    emit messageRequested(1, trimmed, tag, actionPath);
}

void Notifications::dismiss(const QString& tag)
{
    if (tag.isEmpty()) {
        return;
    }
    emit dismissRequested(tag);
}
