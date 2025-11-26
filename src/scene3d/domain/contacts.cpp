#include "contacts.h"

#include "draw_utils.h"
#include "epoch_event.h"
#include "text_renderer.h"

#include "themes.h"
extern Themes theme;


Contacts::Contacts(QObject *parent) :
    SceneObject(new ContactsRenderImplementation, parent),
    datasetPtr_(nullptr)
{

}

Contacts::~Contacts()
{

}

QString Contacts::getContactInfo() const
{
    return info_;
}

bool Contacts::getContactVisible() const
{
    return contactVisible_;
}

int Contacts::getContactPositionX() const
{
    return positionX_;
}

int Contacts::getContactPositionY() const
{
    return positionY_;
}

int Contacts::getContactIndx() const
{
    return indx_;
}

double Contacts::getContactLat() const
{
    return lat_;
}

double Contacts::getContactLon() const
{
    return lon_;
}

double Contacts::getContactDepth() const
{
    return depth_;
}

void Contacts::setContactVisible(bool state)
{
    contactVisible_ = state;
}

void Contacts::clear()
{
    indx_ = -1;
    positionX_ = -1;
    positionY_ = -1;
    info_.clear();
    lat_ = 0.0;
    lon_ = 0.0;
    depth_ = 0.0;

    contactBounds_.clear();
    //datasetPtr_ = nullptr;

    auto* r = RENDER_IMPL(Contacts);
    r->clear();
    r->updateBounds();

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
                cInfo.info = contact.info;
                cInfo.nedPos = { contact.nedX, contact.nedY, -contact.depth };
                cInfo.lat = contact.lat;
                cInfo.lon = contact.lon;
                cInfo.depth = contact.depth;

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

    if (event->type() == ContactActiveChanged) {
        //qDebug() << "EVENT" << epIndx;
        r->activeContactIndx_ = epIndx;
        beenUpdated = true;
    }

    if (beenUpdated) {
        Q_EMIT changed();
    }

    return false;
}

bool Contacts::setContact(int indx, const QString& text)
{
    if (!datasetPtr_) {
        qDebug() << "Contacts::setContact returned: !_dataset";
        return false;
    }

    if (text.isEmpty()) {
        qDebug() << "Contacts::setContact returned: text.isEmpty()";
        return false;
    }

    auto* ep = datasetPtr_->fromIndex(indx);
    if (!ep) {
        qDebug() << "Contacts::setContact returned: !ep";
        return false;
    }

    ep->contact_.info = text;
    //qDebug() << "Plot2D::setContact: setted to epoch:" << indx << text;

    emit datasetPtr_->dataUpdate();

    auto* r = RENDER_IMPL(Contacts);

    if (auto it = r->points_.find(indx); it != r->points_.end()) {
        it.value().info = text;
    }

    setInterEpIndx(-1);

    Q_EMIT changed();

    return true;
}

bool Contacts::deleteContact(int indx)
{
    if (!datasetPtr_) {
        qDebug() << "Contacts::deleteContact: !datasetPtr_";
        return false;
    }

    auto* ep = datasetPtr_->fromIndex(indx);
    if (!ep) {
        qDebug() << "Contacts::deleteContact returned: !ep";
        return false;
    }

    ep->contact_.clear();

    if (datasetPtr_->getActiveContactIndx() == indx) {
        datasetPtr_->setActiveContactIndx(-1);
    }

    emit datasetPtr_->dataUpdate();

    auto* r = RENDER_IMPL(Contacts);
    r->points_.remove(indx);
    r->contactBounds_.remove(indx);

    contactBounds_.remove(indx);

    Q_EMIT changed();

    return true;
}

bool Contacts::setActiveContact(int indx)
{
    if (!datasetPtr_) {
        qDebug() << "Plot2D::setActiveContact returned: !_dataset";
        return false;
    }

    auto* ep = datasetPtr_->fromIndex(indx);
    if (!ep) {
        qDebug() << "Plot2D::setActiveContact returned: !ep";
        return false;
    }

    auto* r = RENDER_IMPL(Contacts);
    auto currActiveIndx = datasetPtr_->getActiveContactIndx();
    if (currActiveIndx == indx) {
        datasetPtr_->setActiveContactIndx(-1);
        r->activeContactIndx_ = -1;
    }
    else {
        datasetPtr_->setActiveContactIndx(indx);
        r->activeContactIndx_ = indx;
    }

    Q_EMIT changed();

    return true;
}

void Contacts::update()
{
    setInterEpIndx(-1);

    Q_EMIT changed();
}

void Contacts::mouseMoveEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons);

    int intersectedEpochIndx = -1;
    QPointF cursorPos(x, y);
    for (auto it = contactBounds_.begin(); it != contactBounds_.end(); ++it) {
        auto rect = it.value();

        if (!std::isfinite(rect.x()) ||
            !std::isfinite(rect.y())) {
            continue;
        }

        if (rect.contains(cursorPos)) {
            intersectedEpochIndx = it.key();
            break;
        }
    }

    if (intersectedEpochIndx != -1) {
        contactVisible_ = true;
        indx_ = intersectedEpochIndx;
        positionX_ = x;
        positionY_ = y;

        if (auto* ep = datasetPtr_->fromIndex(indx_); ep) {
            if (ep->contact_.isValid()) {
                info_ = ep->contact_.info;
                lat_ = ep->contact_.lat;
                lon_ = ep->contact_.lon;
                depth_ = ep->contact_.depth;// echogramDistance;
            }
        }
    }
    else {
        contactVisible_ = false;
    }

    Q_EMIT contactChanged();

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

    if (r->intersectedEpochIndx_ != indx) {
        r->intersectedEpochIndx_ = indx;
        Q_EMIT changed();
    }
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

    float pointSize = 50.0f * theme.getResolutionCoeff();
    int posLoc        = shaderProgram->attributeLocation ("position");
    int matrixLoc     = shaderProgram->uniformLocation   ("matrix");
    int colorLoc      = shaderProgram->uniformLocation   ("color");
    int widthLoc      = shaderProgram->uniformLocation   ("width");
    int isTriangleLoc = shaderProgram->uniformLocation   ("isTriangle");
    int isPointLoc    = shaderProgram->uniformLocation   ("isPoint");

    shaderProgram->enableAttributeArray(posLoc);

    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->setUniformValue(isPointLoc,    false);
    shaderProgram->setUniformValue(isTriangleLoc, true);
    shaderProgram->setUniformValue(widthLoc, pointSize);


    QRectF vport = DrawUtils::viewportRect(ctx);

    ctx->glEnable(34370);
    ctx->glEnable(34913);

    for (auto it = points_.begin(); it != points_.end(); ++it) {
        if (!isfinite(it->lat) || !isfinite(it->lon)) {
            continue;
        }

        QVector3D p = { it.value().nedPos };
        QVector2D pScreen = p.project(view * model, projection, vport.toRect()).toVector2D();
        float correctedY = vport.height() - pScreen.y();
        QRectF markerRect( pScreen.x() - pointSize * 0.125f, correctedY - pointSize * 0.5f, pointSize / 4.0f, pointSize / 2.0f);        
        const_cast<ContactsRenderImplementation*>(this)->contactBounds_.insert(it.key(), markerRect);

        bool isIntersects = it.key() == intersectedEpochIndx_;
        QVector4D markerColor = isIntersects ? QVector4D(0.0f, 0.67f, 0.0f, 1.0f) : (activeContactIndx_ == it.key() ? QVector4D(0.0f, 0.0f, 0.67f, 1.0f) : QVector4D(0.67f, 0.0f, 0.0f, 1.0f));
        shaderProgram->setUniformValue(colorLoc, markerColor);
        QVector<QVector3D> vecP = { p };
        shaderProgram->setAttributeArray(posLoc, vecP.constData());
        ctx->glDrawArrays(GL_POINTS, 0, vecP.size());
    }

    ctx->glDisable(34370); // GL_PROGRAM_POINT_SIZE
    ctx->glDisable(34913); // GL_POINT_SPRITE

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->setUniformValue(isTriangleLoc, false);
    shaderProgram->setUniformValue(isPointLoc,    false);
    shaderProgram->release();

    // text, textback
    for (auto it = points_.begin(); it != points_.end(); ++it) {
        bool isIntersects = it.key() == intersectedEpochIndx_;

        QVector3D p = it.value().nedPos;
        QVector2D pScreen = p.project(view * model, projection, vport.toRect()).toVector2D();

        pScreen.setX(pScreen.x() + 15.0f);

        if (!isIntersects) {
            pScreen.setY(vport.height() - pScreen.y() - 5);

            QMatrix4x4 textProjection;
            textProjection.ortho(vport.toRect());

            TextRenderer::instance().render(it.value().info, 0.9f, pScreen, true, ctx, textProjection, shaderProgramMap);
        }
    }
}

void Contacts::ContactsRenderImplementation::clear()
{
    activeContactIndx_ = -1;
    intersectedEpochIndx_ = -1;
    points_.clear();
    contactBounds_.clear();
}
