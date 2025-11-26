/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "kogger_logging_category.h"

#include <QtCore/QGlobalStatic>
#include <QtCore/QSettings>

KOGGER_LOGGING_CATEGORY(KoggerLoggingCategoryRegisterLog, "Utilities.KoggerLoggingCategoryManager")

Q_GLOBAL_STATIC(KoggerLoggingCategoryManager, _KoggerLoggingCategoryManagerInstance);

KoggerLoggingCategoryManager *KoggerLoggingCategoryManager::instance()
{
    return _KoggerLoggingCategoryManagerInstance();
}

void KoggerLoggingCategoryManager::_insertSorted(QmlObjectListModel* model, KoggerLoggingCategoryItem* item)
{
    for (int i=0; i<model->count(); i++) {
        auto existingItem = qobject_cast<KoggerLoggingCategoryItem*>(model->get(i));
        if (item->fullCategory < existingItem->fullCategory) {
            model->insert(i, item);
            return;
        }
    }
    model->append(item);
}

void KoggerLoggingCategoryManager::registerCategory(const QString &fullCategory) 
{ 
    //qDebug() << "Registering logging full category" << fullCategory;

    QString parentCategory;
    QString childCategory(fullCategory);
    auto currentParentModel = &_treeCategoryModel;

    auto hierarchyIndex = fullCategory.indexOf(".");
    if (hierarchyIndex != -1) {
        parentCategory = fullCategory.left(hierarchyIndex);
        childCategory = fullCategory.mid(hierarchyIndex + 1);
        QString fullParentCategory = parentCategory + ".";
        //qDebug() << "  Parent category" << parentCategory << "child category" << childCategory << "full parent category" << fullParentCategory;
        
        bool found = false;
        for (int j=0; j<currentParentModel->count(); j++) {
            auto item = qobject_cast<KoggerLoggingCategoryItem*>(currentParentModel->get(j));
            if (item->fullCategory == fullParentCategory && item->children) {
                //qDebug() << "  Found existing parent full category" << item->fullCategory;
                currentParentModel = item->children;
                found = true;
                break;
            }
        }
        if (!found) {
            auto newParentItem = new KoggerLoggingCategoryItem(parentCategory, fullParentCategory, false /* enabled */, currentParentModel);
            newParentItem->children = new QmlObjectListModel(newParentItem);
            _insertSorted(&_flatCategoryModel, newParentItem);
            _insertSorted(currentParentModel, newParentItem);
            currentParentModel = newParentItem->children;
            //qDebug() << "  New parent full category" << newParentItem->fullCategory;
        }
    }

    auto categoryItem = new KoggerLoggingCategoryItem(childCategory, fullCategory, false /* enabled */, currentParentModel);
    _insertSorted(&_flatCategoryModel, categoryItem);
    _insertSorted(currentParentModel, categoryItem);
    //qDebug() << "  New category full category" << categoryItem->fullCategory << "childCategory" << childCategory;
}

void KoggerLoggingCategoryManager::setCategoryLoggingOn(const QString &fullCategoryName, bool enable)
{
    qCDebug(KoggerLoggingCategoryRegisterLog) << "Set category logging" << fullCategoryName << enable;
    
    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    if (enable) {
        settings.setValue(fullCategoryName, enable);
    } else {
        settings.remove(fullCategoryName);
    }

    setFilterRulesFromSettings(QString());
}

bool KoggerLoggingCategoryManager::categoryLoggingOn(const QString &fullCategoryName)
{
    QSettings settings;

    settings.beginGroup(kFilterRulesSettingsGroup);
    return settings.value(fullCategoryName, false).toBool();
}

void KoggerLoggingCategoryManager::setFilterRulesFromSettings(const QString &commandLineLoggingOptions)
{
    QString filterRules;
    QString filterRuleFormat("%1.debug=true\n");

    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    for (const QString &fullCategoryName : settings.childKeys()) {
        QString parentCategory;
        QString childCategory;
        _splitFullCategoryName(fullCategoryName, parentCategory, childCategory);

        qCDebug(KoggerLoggingCategoryRegisterLog) << "Setting filter rule for saved settings" << fullCategoryName << parentCategory << childCategory << settings.value(fullCategoryName).toBool();

        auto categoryItem = _findLoggingCategory(fullCategoryName);
        if (categoryItem) {
            categoryItem->setEnabled(settings.value(fullCategoryName).toBool());
            if (categoryItem->enabled()) {
                if (childCategory.isEmpty()) {
                    // Wildcard parent category
                    filterRules += filterRuleFormat.arg(fullCategoryName + "*");
                } else {
                    filterRules += filterRuleFormat.arg(fullCategoryName);
                }
            }
        } else {
            qCWarning(KoggerLoggingCategoryRegisterLog) << "Category not found for saved settings" << fullCategoryName;
        }
    }

    // Command line rules take precedence, so they go last in the list
    if (!commandLineLoggingOptions.isEmpty()) {
        const QStringList categoryList = commandLineLoggingOptions.split(",");

        if (categoryList[0] == QStringLiteral("full")) {
            filterRules += filterRuleFormat.arg("*");
        } else {
            for (const QString &category: categoryList) {
                filterRules += filterRuleFormat.arg(category);
            }
        }
    }

    filterRules += QStringLiteral("qt.qml.connections=false");

    qCDebug(KoggerLoggingCategoryRegisterLog) << "Filter rules" << filterRules;
    QLoggingCategory::setFilterRules(filterRules);
}

void KoggerLoggingCategoryManager::_splitFullCategoryName(const QString &fullCategoryName, QString &parentCategory, QString &childCategory)
{
    const int hierarchyIndex = fullCategoryName.indexOf(".");
    if (hierarchyIndex == -1) {
        parentCategory = QString();
        childCategory = fullCategoryName;
    } else {
        parentCategory = fullCategoryName.left(hierarchyIndex);
        childCategory = fullCategoryName.mid(hierarchyIndex + 1);
    }
}

void KoggerLoggingCategoryManager::disableAllCategories()
{
    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    settings.remove("");

    for (int i=0; i<_flatCategoryModel.count(); i++) {
        auto item = qobject_cast<KoggerLoggingCategoryItem*>(_flatCategoryModel.get(i));
        item->setEnabled(false);
    }

    setFilterRulesFromSettings(QString());
}

KoggerLoggingCategoryItem *KoggerLoggingCategoryManager::_findLoggingCategory(const QString &fullCategoryName)
{
    for (int i=0; i<_flatCategoryModel.count(); i++) {
        auto item = qobject_cast<KoggerLoggingCategoryItem*>(_flatCategoryModel.get(i));
        if (item->fullCategory == fullCategoryName) {
            return item;
        }
    }

    return nullptr;
}

KoggerLoggingCategoryItem::KoggerLoggingCategoryItem(const QString& _shortCategory, const QString& _fullCategory, bool _enabled, QObject* parent)
    : QObject(parent)
    , shortCategory(_shortCategory)
    , fullCategory(_fullCategory)
    , _enabled(_enabled)
{
    connect(this, &KoggerLoggingCategoryItem::enabledChanged, this, [this]() {
        KoggerLoggingCategoryManager::instance()->setCategoryLoggingOn(this->fullCategory, this->_enabled);
    });
}

void KoggerLoggingCategoryItem::setEnabled(bool enabled)
{
    if (enabled != _enabled) {
        _enabled = enabled;
        emit enabledChanged();
    }
}
