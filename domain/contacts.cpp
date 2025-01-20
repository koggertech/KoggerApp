#include "contacts.h"

#include "draw_utils.h"
#include "epochevent.h"


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
        if (!r->indexes_.contains(epIndx)) {
            if (datasetPtr_) {
                if (auto* epoch = datasetPtr_->fromIndex(epIndx); epoch) {
                    auto& contact = epoch->contact_;
                    if (contact.isValid()) {
                        r->indexes_.push_back(epIndx);
                        r->points_.push_back({contact.decX_, contact.decY_, 0});
                        beenUpdated = true;
                    }
                }
            }
        }
    }

    if (event->type() == ContactDeleted) {
        int size = r->indexes_.size();
        for (int i = 0; i < size; ++i) {
            if (epIndx == r->indexes_[i]) {
                r->indexes_.erase(r->indexes_.begin() + i);
                r->points_.erase(r->points_.begin() + i);
                beenUpdated = true;
                break;
            }
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

    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->enableAttributeArray(posLoc);

    // test point
    ctx->glEnable(34370);

    shaderProgram->setUniformValue(isTriangleLoc, true);
    shaderProgram->setUniformValue(colorLoc, QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
    shaderProgram->setUniformValue(widthLoc, 35.f);

    shaderProgram->setAttributeArray(posLoc, points_.constData());
    ctx->glDrawArrays(GL_POINTS, 0, points_.size());

    ctx->glDisable(34370);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->setUniformValue(isTriangleLoc, false);
    shaderProgram->release();

}
