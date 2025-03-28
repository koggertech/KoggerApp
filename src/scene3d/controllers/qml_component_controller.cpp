#include "qml_component_controller.h"

QmlComponentController::QmlComponentController(QObject *parent)
    : QObject{parent}
{}

QmlComponentController::~QmlComponentController()
{}

void QmlComponentController::setQmlEngine(QObject *engine)
{
    m_engine = engine;

    findComponent();
}
