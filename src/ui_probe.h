#pragma once

// Machine-readable UI verification: dump the live QQuickItem tree as JSON and grab the
// window to PNG.
//
// The point is the JSON, not the picture. "The pane is not visible" is a claim about
// numbers -- height, y, parent bounds -- and numbers can be asserted by a script.
// Screenshot-and-squint cannot tell a zero-height item from a clipped one, cannot tell
// which of two overlapping siblings is on top, and cannot be diffed. The PNG is for
// confirming the *look* once the geometry is already known to be sound.
//
// Off unless KOGGER_UI_PROBE names an output directory, so normal runs are unaffected.

#include <QObject>
#include <QString>

class QQuickItem;
class QQuickWindow;
class QJsonObject;

class UiProbe : public QObject
{
    Q_OBJECT
public:
    explicit UiProbe(QObject* parent = nullptr);

    void setWindow(QQuickWindow* window);

    // Arm the env-driven run: KOGGER_UI_PROBE=<dir> writes <dir>/ui-<tag>.json and
    // <dir>/ui-<tag>.png. KOGGER_UI_PROBE_DELAY_MS (default 3000) gives the UI time to
    // settle -- disclosure animations and Flickable geometry are not final at
    // objectCreated. KOGGER_UI_PROBE_QUIT=1 exits afterwards, for a one-shot CI check.
    void armFromEnvironment();

    // Both are callable from QML so a test can dump at a chosen moment rather than on a
    // timer, which is what any real interaction test needs.
    // rootTypeSubstring scopes the dump to the first item whose type matches, which is
    // what keeps the output readable -- the full content item is megabytes of JSON.
    Q_INVOKABLE bool dumpGeometry(const QString& filePath,
                                  const QString& rootObjectName = {},
                                  const QString& rootTypeSubstring = {});
    Q_INVOKABLE bool grabWindow(const QString& filePath);
    Q_INVOKABLE bool dumpAll(const QString& tag, const QString& rootTypeSubstring = {});

    static bool isEnabled();

private:
    QJsonObject describe(QQuickItem* item, int depth, int maxDepth) const;
    QQuickItem* findByType(const QString& typeSubstring) const;

    QQuickWindow* window_ = nullptr;
    QString outDir_;
};
