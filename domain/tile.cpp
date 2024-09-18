#include "tile.h"


Tile::Tile() :
    id_(QUuid::createUuid()),
    textureId_(0),
    isUpdate_(false),
    isInited_(false)
{ }

void Tile::init(int sidePixelSize, int heightMatrixRatio, float resolution, QImage::Format imageFormat)
{
    // image
    image_ = QImage(sidePixelSize, sidePixelSize, imageFormat);
    image_.fill(5);

    // height vertices
    int heightMatSideSize = heightMatrixRatio + 1;
    float heightPixelStep = (sidePixelSize / heightMatrixRatio) * resolution;
    heightVertices_.resize(std::pow(heightMatSideSize, 2));
    for (int i = 0; i < heightMatSideSize; ++i) {
        for (int j = 0; j < heightMatSideSize; ++j) {
            float x = origin_.x() + j * heightPixelStep;
            float y = origin_.y() + i * heightPixelStep;
            heightVertices_[i * heightMatSideSize + j] = QVector3D(x, y, 0.0f);
        }
    }

    // texture vertices
    for (int i = 0; i < heightMatSideSize; ++i) {
        for (int j = 0; j < heightMatSideSize; ++j) {
            textureVertices_.append(QVector2D(float(j) / (heightMatSideSize - 1), float(i) / (heightMatSideSize - 1)));
        }
    }

    gridRenderImpl_.setColor(QColor(0,255,100));
    contourRenderImpl_.setColor(QColor(255,0,0));

    isInited_ = true;
}

void Tile::updateHeightIndices()
{
    // height indices
    heightIndices_.clear();
    int heightMatSideSize = std::sqrt(static_cast<int>(heightVertices_.size()));
    for (int i = 0; i < heightMatSideSize - 1; ++i) { // -1 для норм прохода
        for (int j = 0; j < heightMatSideSize - 1; ++j) {
            int topLeft = i * heightMatSideSize + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * heightMatSideSize + j;
            int bottomRight = bottomLeft + 1;

            if (!checkVerticesDepth(topLeft, topRight, bottomLeft, bottomRight)) {
                continue;
            }

            heightIndices_.append(topLeft);     // 1--3
            heightIndices_.append(bottomLeft);  // | /
            heightIndices_.append(topRight);    // 2
            heightIndices_.append(topRight);    //    1
            heightIndices_.append(bottomLeft);  //  / |
            heightIndices_.append(bottomRight); // 2--3
        }
    }

    // grid
    QVector<QVector3D> grid;
    for (int i = 0; i < heightIndices_.size(); i += 6) {
        QVector3D A = heightVertices_[heightIndices_[i]];
        QVector3D B = heightVertices_[heightIndices_[i + 1]];
        QVector3D C = heightVertices_[heightIndices_[i + 2]];
        QVector3D D = heightVertices_[heightIndices_[i + 5]];
        A.setZ(A.z() + 0.02);
        B.setZ(B.z() + 0.02);
        C.setZ(C.z() + 0.02);
        D.setZ(D.z() + 0.02);
        grid.append({ A, B,
                     B, D,
                     A, C,
                     C, D });
    }
    gridRenderImpl_.setData(grid, GL_LINES);

    // contour
    QVector<QVector3D> contour;
    float zShift = 0.1f;

    // Верхняя граница (первая строка)
    for (int j = 0; j < heightMatSideSize - 1; ++j) {
        QVector3D A = heightVertices_[j];
        QVector3D B = heightVertices_[j + 1];
        A.setZ(A.z() + zShift);
        B.setZ(B.z() + zShift);
        contour.append(A);
        contour.append(B); // линия между A и B
    }

    // Правая граница (последний столбец)
    for (int i = 0; i < heightMatSideSize - 1; ++i) {
        QVector3D A = heightVertices_[i * heightMatSideSize + (heightMatSideSize - 1)];
        QVector3D B = heightVertices_[(i + 1) * heightMatSideSize + (heightMatSideSize - 1)];
        A.setZ(A.z() + zShift);
        B.setZ(B.z() + zShift);
        contour.append(A);
        contour.append(B); // линия между A и B
    }

    // Нижняя граница (последняя строка)
    for (int j = heightMatSideSize - 1; j > 0; --j) {
        QVector3D A = heightVertices_[(heightMatSideSize - 1) * heightMatSideSize + j];
        QVector3D B = heightVertices_[(heightMatSideSize - 1) * heightMatSideSize + (j - 1)];
        A.setZ(A.z() + zShift);
        B.setZ(B.z() + zShift);
        contour.append(A);
        contour.append(B); // линия между A и B
    }

    // Левая граница (первый столбец)
    for (int i = heightMatSideSize - 1; i > 0; --i) {
        QVector3D A = heightVertices_[i * heightMatSideSize];
        QVector3D B = heightVertices_[(i - 1) * heightMatSideSize];
        A.setZ(A.z() + zShift);
        B.setZ(B.z() + zShift);
        contour.append(A);
        contour.append(B); // линия между A и B
    }

    // Установим данные для контурного рендера
    contourRenderImpl_.setData(contour, GL_LINES);
}

void Tile::setOrigin(QVector3D origin)
{
    origin_ = origin;
}

void Tile::setTextureId(GLuint val)
{
    textureId_ = val;
}

void Tile::setIsUpdate(bool val)
{
    isUpdate_ = val;
}

QUuid Tile::getUuid() const
{
    return id_;
}

QVector3D Tile::getOrigin() const
{
    return origin_;
}

bool Tile::getIsInited() const
{
    return isInited_;
}

GLuint Tile::getTextureId() const
{
    return textureId_;
}

int Tile::getIsUpdate() const
{
    return isUpdate_;
}

QImage& Tile::getImageRef()
{
    return image_;
}

QVector<QVector3D>& Tile::getHeightVerticesRef()
{
    return heightVertices_;
}

const QVector<QVector2D>& Tile::getTextureVerticesRef() const
{
    return textureVertices_;
}

const QVector<QVector3D>& Tile::getHeightVerticesRef() const
{
    return heightVertices_;
}

const QVector<int>& Tile::getHeightIndicesRef() const
{
    return heightIndices_;
}

const SceneObject::RenderImplementation& Tile::getGridRenderImplRef() const
{
    return gridRenderImpl_;
}
const SceneObject::RenderImplementation& Tile::getContourRenderImplRef() const
{
    return contourRenderImpl_;
}

bool Tile::checkVerticesDepth(int topLeft, int topRight, int bottomLeft, int bottomRight) const
{
    if (qFuzzyCompare(1.0f, 1.0f + heightVertices_[topLeft].z()) || // someone zero
        qFuzzyCompare(1.0f, 1.0f + heightVertices_[topRight].z()) ||
        qFuzzyCompare(1.0f, 1.0f + heightVertices_[bottomLeft].z()) ||
        qFuzzyCompare(1.0f, 1.0f + heightVertices_[bottomRight].z())) {
        return false;
    }
    return true;
}
