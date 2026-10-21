#pragma once

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class QQuickItem;
class QQuickWindow;

class Tutorial : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList scenarios READ scenarios NOTIFY scenariosChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(QString scenarioId READ scenarioId NOTIFY activeChanged)
    Q_PROPERTY(int stepCount READ stepCount NOTIFY activeChanged)
    Q_PROPERTY(int stepIndex READ stepIndex NOTIFY stepSerialChanged)
    Q_PROPERTY(int stepSerial READ stepSerial NOTIFY stepSerialChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY stepSerialChanged)
    Q_PROPERTY(QVariantMap step READ step NOTIFY stepChanged)
    Q_PROPERTY(QString phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool targetFound READ targetFound NOTIFY targetChanged)
    Q_PROPERTY(QRectF targetRect READ targetRect NOTIFY targetChanged)
    Q_PROPERTY(QStringList loadErrors READ loadErrors CONSTANT)

public:
    explicit Tutorial(QObject* parent = nullptr);

    void loadFromDirectory(const QString& directory);
    void setWindow(QQuickWindow* window);

    QVariantList scenarios() const;
    bool active() const { return activeIndex_ >= 0; }
    QString scenarioId() const;
    int stepCount() const;
    int stepIndex() const { return stepIndex_; }
    int stepSerial() const { return stepSerial_; }
    bool canGoBack() const { return !history_.isEmpty(); }
    QVariantMap step() const { return step_; }
    QString phase() const { return phase_; }
    bool targetFound() const { return targetFound_; }
    QRectF targetRect() const { return targetRect_; }
    QStringList loadErrors() const { return loadErrors_; }

    Q_INVOKABLE bool start(const QString& id);
    Q_INVOKABLE bool resume(const QString& id);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void finish();
    Q_INVOKABLE void advance();
    Q_INVOKABLE void skipStep();
    Q_INVOKABLE void back();
    Q_INVOKABLE bool goTo(const QString& stepId);

    Q_INVOKABLE void stepReady(int serial);
    Q_INVOKABLE void stepSuspended(int serial);
    Q_INVOKABLE void stepFailed(int serial, const QString& reason);
    Q_INVOKABLE void reportTarget(int serial, bool found, const QRectF& rect);

    Q_INVOKABLE void registerAnchor(const QString& key, QQuickItem* item);
    Q_INVOKABLE void unregisterAnchor(const QString& key, QQuickItem* item);
    Q_INVOKABLE QQuickItem* anchor(const QString& key) const;
    Q_INVOKABLE bool hasAnchor(const QString& key) const;

    Q_INVOKABLE void resetProgress(const QString& id = QString());

public slots:
    void retranslate();
    void reloadProgress();

signals:
    void scenariosChanged();
    void activeChanged();
    void stepSerialChanged();
    void stepChanged();
    void phaseChanged();
    void targetChanged();
    void anchorsChanged(const QString& key);
    void finished(const QString& id, bool completed);
    void failed(const QString& id, const QString& reason);

private:
    struct Scenario {
        QString id;
        int version = 1;
        bool devOnly = false;
        QString title;
        QJsonObject defaults;
        QList<QJsonObject> steps;
        QStringList stepIds;
    };

    struct Progress {
        bool completed = false;
        QString checkpoint;
    };

    bool parseScenario(const QString& filePath, Scenario& out, QStringList& errors) const;
    int indexOfScenario(const QString& id) const;
    bool beginScenario(int scenarioIndex, int stepIndex);
    void moveForward(bool pushHistory);
    void enterStep(int index, bool pushHistory);
    void rebuildStep();
    void endScenario(bool completed);
    void setPhase(const QString& phase);
    void resetTarget();

    QString translate(const Scenario& scenario, const QString& source) const;
    Progress readStoredProgress(const Scenario& scenario) const;
    Progress loadProgress(const Scenario& scenario) const;
    void saveCheckpoint(const Scenario& scenario, const QString& stepId);
    void saveCompleted(const Scenario& scenario);

    QList<Scenario> scenarios_;
    QHash<QString, Progress> progress_;
    QStringList loadErrors_;
    QHash<QString, QList<QPointer<QQuickItem>>> anchors_;
    QPointer<QQuickWindow> window_;

    int activeIndex_ = -1;
    int stepIndex_ = -1;
    int stepSerial_ = 0;
    QList<int> history_;
    QVariantMap step_;
    QString phase_ = QStringLiteral("idle");
    bool targetFound_ = false;
    QRectF targetRect_;
};
