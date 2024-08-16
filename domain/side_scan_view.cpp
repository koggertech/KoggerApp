#include "side_scan_view.h"

#include <QtMath>


SideScanView::SideScanView(QObject* parent) :
    SceneObject(new SideScanViewRenderImplementation, parent)
{

}

SideScanView::~SideScanView()
{

}

SideScanView::SideScanViewRenderImplementation::SideScanViewRenderImplementation()
{

}

void SideScanView::SideScanViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

    auto shaderProgram = shaderProgramMap.value("static", nullptr);

    if (!shaderProgram) {
        qWarning() << "Shader program 'static' not found!";
        return;
    }

    shaderProgram->bind();

    shaderProgram->setUniformValue("mvp", mvp);

    int posLoc = shaderProgram->attributeLocation("position");
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_data.constData());

    shaderProgram->setUniformValue("color", QVector4D(0.0f, 1.0f, 0.0f, 1.0f));
    ctx->glDrawElements(GL_LINES, evenIndices_.size(), GL_UNSIGNED_INT, evenIndices_.constData());

    shaderProgram->setUniformValue("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
    ctx->glDrawElements(GL_LINES, oddIndices_.size(), GL_UNSIGNED_INT, oddIndices_.constData());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void SideScanView::updateData()
{
    if (!datasetPtr_) {
        qDebug() << "dataset is nullptr!";
        return;
    }

    auto renderImpl = RENDER_IMPL(SideScanView);
    QVector<QVector3D> vertices;

    if (auto chList = datasetPtr_->channelsList(); chList.size() == 2) {
        auto it = chList.begin();
        int fCh = it.key();
        ++it;
        int sCh = it.key();

        for (int i = 0; i < datasetPtr_->size(); ++i) {
            auto epoch = datasetPtr_->fromIndex(i);

            if (!epoch) {
                continue;
            }

            auto pos = epoch->getPositionGNSS().ned;
            pos.d = 0.0f;
            auto yaw = epoch->yaw();

            auto fChartsPrt = epoch->chart(fCh);
            auto sChartsPrt = epoch->chart(sCh);

            if (isfinite(pos.n) && isfinite(pos.e) && isfinite(yaw)) {
                double azimuthRad = qDegreesToRadians(yaw);

                if (fChartsPrt) {
                    double leftAzimuthRad = azimuthRad + M_PI_2;

                    float lDistance = fChartsPrt->range();
                    float lX = pos.n + lDistance * qCos(leftAzimuthRad);
                    float lY = pos.e + lDistance * qSin(leftAzimuthRad);
                    float lZ = pos.d;
                    vertices.append(QVector3D(lX, lY, lZ));
                    vertices.append(QVector3D(pos.n, pos.e, pos.d));
                }

                if (sChartsPrt) {
                    double rightAzimuthRad = azimuthRad - M_PI_2;

                    float rDistance = sChartsPrt->range();
                    float rX = pos.n + rDistance * qCos(rightAzimuthRad);
                    float rY = pos.e + rDistance * qSin(rightAzimuthRad);
                    float rZ = pos.d;
                    vertices.append(QVector3D(pos.n, pos.e, pos.d));
                    vertices.append(QVector3D(rX, rY, rZ));
                }
            }
        }
    }

    renderImpl->m_data = vertices;

    QVector<int> evenIndices;
    QVector<int> oddIndices;

    for (int i = 0; i < vertices.size(); i += 2) {
        if (i + 1 < vertices.size()) {
            if ((i / 2) % 2 == 0) {
                evenIndices.append(i);
                evenIndices.append(i + 1);
            } else {
                oddIndices.append(i);
                oddIndices.append(i + 1);
            }
        }
    }

    renderImpl->evenIndices_ = evenIndices;
    renderImpl->oddIndices_ = oddIndices;

    Q_EMIT changed();
}

void SideScanView::clear()
{
    auto renderImpl = RENDER_IMPL(SideScanView);

    renderImpl->evenIndices_.clear();
    renderImpl->oddIndices_.clear();
    renderImpl->m_data.clear();

    Q_EMIT changed();
}

void SideScanView::setDatasetPtr(Dataset* datasetPtr)
{
    datasetPtr_ = datasetPtr;
}
