#include "hotkeys_controller.h"

#include <QQmlContext>


HotkeysController::HotkeysController(QQmlApplicationEngine* engine, QObject* parent)
    : QObject(parent)
    , engine_(engine)
{
}

bool HotkeysController::updateHotkey(const QString& functionName, quint32 newScanCode, quint32 parameter)
{
    if (!manager_.updateHotkey(functionName, newScanCode, parameter))
        return false;

    refreshContextProperties();
    emit hotkeysUpdated();
    return true;
}

bool HotkeysController::resetToDefaults()
{
    if (!manager_.resetToDefaults())
        return false;

    refreshContextProperties();
    emit hotkeysUpdated();
    return true;
}

void HotkeysController::refreshContextProperties()
{
    auto hotkeysMap     = manager_.loadHotkeysMapping();
    auto hotkeysVariant = HotkeysManager::toVariantMap(hotkeysMap);
    auto displayList    = HotkeysManager::toDisplayVariantList(manager_.loadHotkeysList());

    engine_->rootContext()->setContextProperty("hotkeysMapScan",     hotkeysVariant);
    engine_->rootContext()->setContextProperty("hotkeysDisplayList", displayList);
}
