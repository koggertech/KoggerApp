#include "bottomtrack.h"

#include <constants.h>

BottomTrack::BottomTrack(QObject* parent)
    : DisplayedObject(GL_LINE_STRIP, parent)
{

}

BottomTrack::~BottomTrack()
{

}

void BottomTrack::setFilter(std::shared_ptr<AbstractBottomTrackFilter> filter)
{
    if(mpFilter == filter)
        return;

    mpFilter = filter;

    Q_EMIT filterChanged(mpFilter.get());
}

float BottomTrack::routeLength() const
{
    return 0.0f;
}

AbstractBottomTrackFilter *BottomTrack::filter() const
{
    return mpFilter.get();
}

void BottomTrack::setData(const QVector<QVector3D> &data)
{
    QVector <QVector3D> filtered;

    if (mpFilter){
        mpFilter->apply(data,filtered);
        DisplayedObject::setData(filtered);
        return;
    }

    DisplayedObject::setData(data);
}

SceneObject::SceneObjectType BottomTrack::type() const
{
    return SceneObjectType::BottomTrack;
}

void BottomTrack::draw(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, QMap <QString, QOpenGLShaderProgram*> shaderProgramMap) const
{
    if(!mIsVisible)
        return;

    if(!shaderProgramMap.contains("height"))
        return;

    auto shaderProgram = shaderProgramMap["height"];

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int maxZLoc   = shaderProgram->uniformLocation("max_z");
    int minZLoc   = shaderProgram->uniformLocation("min_z");
    int matrixLoc = shaderProgram->uniformLocation("matrix");

    QVector4D color(0.8f, 0.2f, 0.7f, 1.0f);
    int colorLoc = shaderProgram->uniformLocation("color");

    shaderProgram->setUniformValue(colorLoc,color);
    shaderProgram->setUniformValue(maxZLoc, mBounds.maximumZ());
    shaderProgram->setUniformValue(minZLoc, mBounds.minimumZ());
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, mData.constData());

    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(mPrimitiveType, 0, mData.size());
    ctx->glLineWidth(1.0);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
    shaderProgram->release();
}
