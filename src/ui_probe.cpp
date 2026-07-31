#include "ui_probe.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml/QQmlProperty>

namespace {

constexpr int kMaxDepth = 40;

// Positioner/layout children are laid out by the parent, so which of `height` and
// `implicitHeight` is authoritative differs by parent type. Recording the parent's type
// is what makes a height of 0 diagnosable rather than merely visible.
QString typeNameOf(const QObject* o)
{
    if (!o) {
        return QStringLiteral("null");
    }
    QString n = QString::fromLatin1(o->metaObject()->className());
    // QML types arrive as Foo_QMLTYPE_123 / QQuickColumn; keep the useful part.
    const int marker = n.indexOf(QStringLiteral("_QMLTYPE_"));
    if (marker > 0) {
        n.truncate(marker);
    }
    const int qmlBase = n.indexOf(QStringLiteral("_QML_"));
    if (qmlBase > 0) {
        n.truncate(qmlBase);
    }
    return n;
}

// Read a property only if the item actually declares it: probing a QML-declared property
// that does not exist must not fabricate a value, or the dump lies.
QJsonValue optionalProperty(QQuickItem* item, const char* name)
{
    const QQmlProperty p(item, QString::fromLatin1(name));
    if (!p.isValid()) {
        return QJsonValue();
    }
    const QVariant v = p.read();
    switch (v.metaType().id()) {
    case QMetaType::Bool:    return v.toBool();
    case QMetaType::Int:     return v.toInt();
    case QMetaType::Double:
    case QMetaType::Float:   return v.toDouble();
    case QMetaType::QString: return v.toString();
    default:                 return QJsonValue();
    }
}

} // namespace

UiProbe::UiProbe(QObject* parent) : QObject(parent) {}

bool UiProbe::isEnabled()
{
    return !qEnvironmentVariable("KOGGER_UI_PROBE").isEmpty();
}

void UiProbe::setWindow(QQuickWindow* window)
{
    window_ = window;
}

void UiProbe::armFromEnvironment()
{
    outDir_ = qEnvironmentVariable("KOGGER_UI_PROBE");
    if (outDir_.isEmpty()) {
        return;
    }
    QDir().mkpath(outDir_);

    bool ok = false;
    int delay = qEnvironmentVariable("KOGGER_UI_PROBE_DELAY_MS").toInt(&ok);
    if (!ok || delay < 0) {
        delay = 3000;
    }

    const bool quitAfter = qEnvironmentVariable("KOGGER_UI_PROBE_QUIT") == QLatin1String("1");

    // A fixed delay only works if the thing you want to inspect happens to be on screen
    // when it fires. KOGGER_UI_PROBE_WAIT_FOR=<TypeSubstring> instead polls until an item
    // of that type exists, then dumps -- so the operator opens the pane once and the
    // capture takes itself, with no guessing about timing.
    const QString waitFor = qEnvironmentVariable("KOGGER_UI_PROBE_WAIT_FOR");
    if (!waitFor.isEmpty()) {
        bool ok2 = false;
        int timeout = qEnvironmentVariable("KOGGER_UI_PROBE_TIMEOUT_MS").toInt(&ok2);
        if (!ok2 || timeout <= 0) {
            timeout = 120000;
        }
        // Do NOT start walking immediately. Traversing the item tree while the scene
        // graph is still being constructed segfaults; the same walk at 6 s is fine. A
        // grace period is the fix, not a different traversal.
        bool ok3 = false;
        int grace = qEnvironmentVariable("KOGGER_UI_PROBE_GRACE_MS").toInt(&ok3);
        if (!ok3 || grace < 2000) {
            grace = 6000;
        }

        QTimer::singleShot(grace, this, [this, waitFor, timeout, quitAfter, delay]() {
            auto* poll = new QTimer(this);
            auto* elapsed = new int(0);
            poll->setInterval(500);
            connect(poll, &QTimer::timeout, this, [this, poll, elapsed, waitFor, timeout,
                                                   quitAfter, delay]() {
                *elapsed += poll->interval();
                if (findByType(waitFor)) {
                    poll->stop();
                    // One more beat: the item exists but disclosure animations and
                    // Flickable geometry are still settling, and a mid-animation height
                    // is not a bug.
                    QTimer::singleShot(delay, this, [this, quitAfter, waitFor]() {
                        qInfo("ui-probe: '%s' appeared, capturing", qPrintable(waitFor));
                        dumpAll(QStringLiteral("auto"), waitFor);
                        if (quitAfter) {
                            QGuiApplication::quit();
                        }
                    });
                } else if (*elapsed >= timeout) {
                    poll->stop();
                    qWarning("ui-probe: timed out after %d ms waiting for '%s'; "
                             "capturing the whole tree instead", timeout,
                             qPrintable(waitFor));
                    dumpAll(QStringLiteral("timeout"));
                    if (quitAfter) {
                        QGuiApplication::quit();
                    }
                }
            });
            poll->start();
            qInfo("ui-probe: polling for '%s' (timeout %d ms)", qPrintable(waitFor),
                  timeout);
        });
        qInfo("ui-probe: armed, %d ms grace before polling for '%s'", grace,
              qPrintable(waitFor));
        return;
    }

    QTimer::singleShot(delay, this, [this, quitAfter]() {
        dumpAll(QStringLiteral("auto"));
        if (quitAfter) {
            QGuiApplication::quit();
        }
    });
}

QQuickItem* UiProbe::findByType(const QString& typeSubstring) const
{
    if (!window_ || !window_->contentItem()) {
        return nullptr;
    }
    QList<QQuickItem*> stack{window_->contentItem()};
    while (!stack.isEmpty()) {
        QQuickItem* it = stack.takeLast();
        if (typeNameOf(it).contains(typeSubstring, Qt::CaseInsensitive)) {
            return it;
        }
        stack.append(it->childItems());
    }
    return nullptr;
}

bool UiProbe::dumpAll(const QString& tag, const QString& rootTypeSubstring)
{
    if (outDir_.isEmpty()) {
        outDir_ = qEnvironmentVariable("KOGGER_UI_PROBE");
    }
    if (outDir_.isEmpty()) {
        return false;
    }
    qInfo("ui-probe: dumping geometry (tag=%s)", qPrintable(tag));
    // Scoping to a subtree keeps the dump readable: the whole content item is ~14 MB of
    // JSON, and a layout question is always about one page.
    const bool a = dumpGeometry(outDir_ + QStringLiteral("/ui-") + tag
                                + QStringLiteral(".json"), {}, rootTypeSubstring);

    // The PNG is opt-in. grabWindow() has to round-trip the scene graph, and this app
    // drives a custom GL scene (GraphicsScene3dView) under the OpenGL RHI, where that is
    // not reliably safe from the GUI thread. The geometry JSON is the part that actually
    // answers layout questions, so it must never be at risk from the screenshot.
    if (qEnvironmentVariable("KOGGER_UI_PROBE_PNG") != QLatin1String("1")) {
        return a;
    }
    qInfo("ui-probe: grabbing window");
    const bool b = grabWindow(outDir_ + QStringLiteral("/ui-") + tag
                              + QStringLiteral(".png"));
    return a && b;
}

QJsonObject UiProbe::describe(QQuickItem* item, int depth, int maxDepth) const
{
    QJsonObject o;
    o[QStringLiteral("type")] = typeNameOf(item);
    if (!item->objectName().isEmpty()) {
        o[QStringLiteral("name")] = item->objectName();
    }

    o[QStringLiteral("x")] = item->x();
    o[QStringLiteral("y")] = item->y();
    o[QStringLiteral("w")] = item->width();
    o[QStringLiteral("h")] = item->height();
    o[QStringLiteral("implicitW")] = item->implicitWidth();
    o[QStringLiteral("implicitH")] = item->implicitHeight();
    o[QStringLiteral("visible")] = item->isVisible();
    // isVisible() folds in every ancestor; the local flag is what the QML actually set,
    // and telling them apart is the difference between "this group is hidden" and "its
    // parent is".
    o[QStringLiteral("visibleSelf")] = item->property("visible").toBool();
    o[QStringLiteral("opacity")] = item->opacity();
    o[QStringLiteral("enabled")] = item->isEnabled();
    o[QStringLiteral("clip")] = item->clip();
    if (item->z() != 0.0) {
        o[QStringLiteral("z")] = item->z();
    }

    // Absolute position in the window: the only coordinate space in which overlap
    // between items from different subtrees can be checked.
    const QPointF scenePos = item->mapToScene(QPointF(0, 0));
    o[QStringLiteral("sceneX")] = scenePos.x();
    o[QStringLiteral("sceneY")] = scenePos.y();

    for (const char* p : {"title", "stateKey", "expanded", "collapsedByDefault",
                          "preferredWidth", "contentPadding", "spacing"}) {
        const QJsonValue v = optionalProperty(item, p);
        if (!v.isNull() && !v.isUndefined()) {
            o[QString::fromLatin1(p)] = v;
        }
    }

    const QList<QQuickItem*> kids = item->childItems();
    if (!kids.isEmpty() && depth < maxDepth) {
        QJsonArray arr;
        for (QQuickItem* k : kids) {
            if (k) {
                arr.append(describe(k, depth + 1, maxDepth));
            }
        }
        o[QStringLiteral("children")] = arr;
    } else if (!kids.isEmpty()) {
        o[QStringLiteral("childrenTruncated")] = kids.size();
    }
    return o;
}

bool UiProbe::dumpGeometry(const QString& filePath, const QString& rootObjectName,
                           const QString& rootTypeSubstring)
{
    if (!window_) {
        qWarning("ui-probe: no window set");
        return false;
    }
    QQuickItem* root = window_->contentItem();
    if (!root) {
        qWarning("ui-probe: window has no content item");
        return false;
    }

    if (!rootObjectName.isEmpty()) {
        QQuickItem* found = root->findChild<QQuickItem*>(rootObjectName);
        if (!found) {
            qWarning("ui-probe: no item named '%s'", qPrintable(rootObjectName));
            return false;
        }
        root = found;
    } else if (!rootTypeSubstring.isEmpty()) {
        if (QQuickItem* found = findByType(rootTypeSubstring)) {
            root = found;
        } else {
            qWarning("ui-probe: no item of type ~'%s'; dumping the whole tree",
                     qPrintable(rootTypeSubstring));
        }
    }

    QJsonObject doc;
    doc[QStringLiteral("window")] = QJsonObject{
        {QStringLiteral("w"), window_->width()},
        {QStringLiteral("h"), window_->height()},
        {QStringLiteral("devicePixelRatio"), window_->devicePixelRatio()},
    };
    doc[QStringLiteral("root")] = describe(root, 0, kMaxDepth);

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning("ui-probe: cannot write %s", qPrintable(filePath));
        return false;
    }
    f.write(QJsonDocument(doc).toJson(QJsonDocument::Indented));
    f.close();
    qInfo("ui-probe: geometry -> %s", qPrintable(filePath));
    return true;
}

bool UiProbe::grabWindow(const QString& filePath)
{
    if (!window_) {
        return false;
    }
    // grabWindow() must run on the GUI thread with the scene graph alive; it returns a
    // null image if called before the first frame.
    const QImage img = window_->grabWindow();
    if (img.isNull()) {
        qWarning("ui-probe: grabWindow() returned a null image (no frame rendered yet?)");
        return false;
    }
    if (!img.save(filePath, "PNG")) {
        qWarning("ui-probe: cannot save %s", qPrintable(filePath));
        return false;
    }
    qInfo("ui-probe: screenshot -> %s (%dx%d)", qPrintable(filePath), img.width(),
          img.height());
    return true;
}
