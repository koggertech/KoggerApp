#ifndef QMLCOMPONENTCONTROLLER_H
#define QMLCOMPONENTCONTROLLER_H

#include <QObject>

class QmlComponentController : public QObject
{
    Q_OBJECT

public:
    explicit QmlComponentController(QObject *parent = nullptr);
    virtual ~QmlComponentController();

public Q_SLOTS:
    virtual void setQmlEngine(QObject* engine);

protected:
    virtual void findComponent() = 0;

protected:
    QObject* m_engine = nullptr;
    QObject* m_component = nullptr;
};

#endif // QMLCOMPONENTCONTROLLER_H
