#include "contacts.h"

#include "draw_utils.h"
#include "epochevent.h"
#include "textrenderer.h"


Contacts::Contacts(QObject *parent) :
    SceneObject(new ContactsRenderImplementation, parent),
    datasetPtr_(nullptr)
{

}

Contacts::~Contacts()
{

}

void Contacts::clear()
{
    contactBounds_.clear();
    //datasetPtr_ = nullptr;

    auto* r = RENDER_IMPL(Contacts);
    r->clear();
    r->createBounds();

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void Contacts::setDatasetPtr(Dataset* datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

bool Contacts::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    bool beenUpdated = false;
    auto r = RENDER_IMPL(Contacts);
    auto* epochEvent = static_cast<EpochEvent*>(event);
    int epIndx = epochEvent->epochIndex();

    if (event->type() == ContactCreated) {
        if (!datasetPtr_) {
            return false;
        }

        if (auto* epoch = datasetPtr_->fromIndex(epIndx); epoch) {
            auto& contact = epoch->contact_;
            if (contact.isValid()) {
                ContactInfo cInfo;
                cInfo.info = contact.info_;
                cInfo.nedPos = { contact.decX_, contact.decY_, -contact.distance_ };
                cInfo.lat = contact.lat_;
                cInfo.lon = contact.lon_;
                cInfo.depth = contact.distance_;

                r->points_.insert(epIndx, cInfo);
                beenUpdated = true;
            }
        }
    }

    if (event->type() == ContactDeleted) {
        if (r->points_.remove(epIndx)) {
            beenUpdated = true;
        }
    }

    if (beenUpdated) {
        Q_EMIT changed();
    }

    return false;
}

void Contacts::mouseMoveEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons);

    int intersectedEpochIndx = -1;
    QPointF cursorPos(x, y);
    for (auto it = contactBounds_.begin(); it != contactBounds_.end(); ++it) {
        if (it.value().contains(cursorPos)) {
            intersectedEpochIndx = it.key();
            break;
        }
    }

    setInterEpIndx(intersectedEpochIndx);
}

void Contacts::mousePressEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons);
    Q_UNUSED(x);
    Q_UNUSED(y);
    //qDebug() << "Contacts::mousePressEvent" << buttons << x << y;
}

void Contacts::mouseReleaseEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons);
    Q_UNUSED(x);
    Q_UNUSED(y);
    //qDebug() << "Contacts::mouseReleaseEvent" << buttons << x << y;
}

void Contacts::mouseWheelEvent(Qt::MouseButtons buttons, qreal x, qreal y, QPointF angleDelta)
{
    Q_UNUSED(buttons);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(angleDelta);
   // qDebug() << "Contacts::mouseWheelEvent" << buttons << x << y << angleDelta;
}

void Contacts::keyPressEvent(Qt::Key key)
{
    Q_UNUSED(key);
   // qDebug() << "Contacts::keyPressEvent" << key;
}

void Contacts::setInterEpIndx(int indx)
{
    auto* r = RENDER_IMPL(Contacts);
    r->intersectedEpochIndx_ = indx;
    Q_EMIT changed();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// ContactsRenderImplementation
Contacts::ContactsRenderImplementation::ContactsRenderImplementation() :
    intersectedEpochIndx_(-1)
{

}

void Contacts::ContactsRenderImplementation::render(QOpenGLFunctions *ctx,
                                                    const QMatrix4x4 &model,
                                                    const QMatrix4x4 &view,
                                                    const QMatrix4x4 &projection,
                                                    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    // marker
    auto shaderProgram = shaderProgramMap.value("static", nullptr);

    if (!shaderProgram) {
        qWarning() << "Shader program 'static' not found!";
        return;
    }

    shaderProgram->bind();

    float pointSize = 50.0f;
    int posLoc        = shaderProgram->attributeLocation ("position");
    int matrixLoc     = shaderProgram->uniformLocation   ("matrix");
    int colorLoc      = shaderProgram->uniformLocation   ("color");
    int widthLoc      = shaderProgram->uniformLocation   ("width");
    int isTriangleLoc = shaderProgram->uniformLocation   ("isTriangle");

    shaderProgram->enableAttributeArray(posLoc);

    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->setUniformValue(isTriangleLoc, true);
    shaderProgram->setUniformValue(widthLoc, pointSize);


    QRectF vport = DrawUtils::viewportRect(ctx);

    ctx->glEnable(34370);

    for (auto it = points_.begin(); it != points_.end(); ++it) {
        QVector3D p = { it.value().nedPos };
        QVector2D pScreen = p.project(view * model, projection, vport.toRect()).toVector2D();
        float correctedY = vport.height() - pScreen.y();
        QRectF markerRect( pScreen.x() - pointSize * 0.125f, correctedY - pointSize * 0.5f, pointSize / 4.0f, pointSize / 2.0f);
        const_cast<ContactsRenderImplementation*>(this)->contactBounds_.insert(it.key(), markerRect);

        bool isIntersects = it.key() == intersectedEpochIndx_;
        shaderProgram->setUniformValue(colorLoc, isIntersects ? QVector4D(0.0f, 1.0f, 0.0f, 1.0f) : QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
        QVector<QVector3D> vecP = { p };
        shaderProgram->setAttributeArray(posLoc, vecP.constData());
        ctx->glDrawArrays(GL_POINTS, 0, vecP.size());
    }

    ctx->glDisable(34370);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->setUniformValue(isTriangleLoc, false);
    shaderProgram->release();

    // text, textback
    for (auto it = points_.begin(); it != points_.end(); ++it) {
        bool isIntersects = it.key() == intersectedEpochIndx_;

        QVector3D p = it.value().nedPos;
        QVector2D pScreen = p.project(view * model, projection, vport.toRect()).toVector2D();

        pScreen.setX(pScreen.x() + 15.0f);

        if (isIntersects) {
            pScreen.setY(vport.height() - pScreen.y() - 25);

            QMatrix4x4 textProjection;
            textProjection.ortho(vport.toRect());

            TextRenderer::instance().render(it.value().info, 1.0f, pScreen, true, ctx, textProjection, shaderProgramMap);

            pScreen.setY(pScreen.y() + 20);
            QString coordStr = "lat: " + QString::number(it.value().lat,'f',4);
            TextRenderer::instance().render(coordStr, 0.8f, pScreen, true, ctx, textProjection, shaderProgramMap);
            coordStr = "lon: " + QString::number(it.value().lon,'f',4);
            pScreen.setY(pScreen.y() + 20);
            TextRenderer::instance().render(coordStr, 0.8f, pScreen, true, ctx, textProjection, shaderProgramMap);
            coordStr = "depth: " + QString::number(it.value().depth, 'f', 4);
            pScreen.setY(pScreen.y() + 20);
            TextRenderer::instance().render(coordStr, 0.8f, pScreen, true, ctx, textProjection, shaderProgramMap);
        }
        else {
            pScreen.setY(vport.height() - pScreen.y() - 5);

            QMatrix4x4 textProjection;
            textProjection.ortho(vport.toRect());

            TextRenderer::instance().render(it.value().info, 1.0f, pScreen, true, ctx, textProjection, shaderProgramMap);
        }
    }
}

void Contacts::ContactsRenderImplementation::clear()
{
    intersectedEpochIndx_ = -1;
    points_.clear();
}
