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

    auto renderImpl = RENDER_IMPL(Contacts);

    //

    renderImpl->createBounds();


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
                r->points_.insert(epIndx, { {contact.decX_, contact.decY_, 0}, contact.info_});
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

/////////////////////////////////////////////////////////////////////////////////////////////////
// ContactsRenderImplementation
Contacts::ContactsRenderImplementation::ContactsRenderImplementation()
{

}

void Contacts::ContactsRenderImplementation::render(QOpenGLFunctions *ctx,
                                                    const QMatrix4x4 &model,
                                                    const QMatrix4x4 &view,
                                                    const QMatrix4x4 &projection,
                                                    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    // points
    auto shaderProgram = shaderProgramMap.value("static", nullptr);

    if (!shaderProgram) {
        qWarning() << "Shader program 'static' not found!";
        return;
    }

    shaderProgram->bind();

    int posLoc     = shaderProgram->attributeLocation ("position");
    int matrixLoc  = shaderProgram->uniformLocation   ("matrix");
    int colorLoc   = shaderProgram->uniformLocation   ("color");
    int widthLoc   = shaderProgram->uniformLocation   ("width");
    int isTriangleLoc = shaderProgram->uniformLocation   ("isTriangle");

    shaderProgram->enableAttributeArray(posLoc);

    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->setUniformValue(isTriangleLoc, true);
    shaderProgram->setUniformValue(colorLoc, QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
    shaderProgram->setUniformValue(widthLoc, 35.f);

    ctx->glEnable(34370);

    for (auto it = points_.begin(); it != points_.end(); ++it) {
        QVector<QVector3D> point = { it.value().first };
        shaderProgram->setAttributeArray(posLoc, point.constData());
        ctx->glDrawArrays(GL_POINTS, 0, point.size());
    }

    ctx->glDisable(34370);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->setUniformValue(isTriangleLoc, false);
    shaderProgram->release();


    // text on points
    for (auto it = points_.begin(); it != points_.end(); ++it) {
        // test text
        QVector3D p = it.value().first;
        QRectF vport = DrawUtils::viewportRect(ctx);

        QVector2D p_screen = p.project(view * model, projection, vport.toRect()).toVector2D();
        p_screen.setY(vport.height() - p_screen.y());

        QMatrix4x4 textProjection;
        textProjection.ortho(vport.toRect());

        TextRenderer::instance().render(it.value().second,
                                        0.3f,
                                        p_screen,
                                        ctx,
                                        textProjection);
    }
}
