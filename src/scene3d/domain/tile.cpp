#include "tile.h"


Tile::Tile(QVector3D origin, bool generateGridContour) :
    id_(QUuid::createUuid()),
    origin_(origin),
    textureId_(0),
    isUpdate_(false),
    isInited_(false),
    generateGridContour_(generateGridContour)
{ }

void Tile::init(int sidePixelSize, int heightMatrixRatio, float resolution)
{
    // image data
    imageData_.resize(sidePixelSize * sidePixelSize, 0);

    // height vertices
    int heightMatSideSize = heightMatrixRatio + 1;
    float heightPixelStep = (sidePixelSize / heightMatrixRatio) * resolution;
    int heightMatSize = std::pow(heightMatSideSize, 2);
    heightVertices_.resize(heightMatSize);
    heightMarkVertices_.resize(heightMatSize);
    for (int i = 0; i < heightMatSideSize; ++i) {
        for (int j = 0; j < heightMatSideSize; ++j) {
            float x = origin_.x() + j * heightPixelStep;
            float y = origin_.y() + i * heightPixelStep;
            int currIndx = i * heightMatSideSize + j;
            heightVertices_[currIndx] = QVector3D(x, y, 0.0f);
            heightMarkVertices_[currIndx] = '0';
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

    if (generateGridContour_) {
        // grid
        QVector<QVector3D> grid;
        grid.reserve((heightIndices_.size() / 6) * 8);
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
        float zShift = 0.1f;
        int lastIndex = heightMatSideSize - 1;
        QVector<QVector3D> contour;
        contour.reserve(lastIndex * 8);
        auto addContourLine = [&](QVector3D A, QVector3D B) {
            A.setZ(A.z() + zShift);
            B.setZ(B.z() + zShift);
            contour.append(A);
            contour.append(B);
        };
        for (int i = 0; i < lastIndex; ++i) {
            addContourLine(heightVertices_[i], heightVertices_[i + 1]); // top
            addContourLine(heightVertices_[lastIndex * heightMatSideSize + i], // bottom
                           heightVertices_[lastIndex * heightMatSideSize + (i + 1)]);
            addContourLine(heightVertices_[(i + 1) * heightMatSideSize], heightVertices_[i * heightMatSideSize]); // left
            addContourLine(heightVertices_[i * heightMatSideSize + lastIndex], // right
                           heightVertices_[(i + 1) * heightMatSideSize + lastIndex]);
        }
        contourRenderImpl_.setData(contour, GL_LINES);
    }
}

void Tile::setTextureId(GLuint val)
{
    textureId_ = val;
}

void Tile::setIsUpdate(bool state)
{
    isUpdate_ = state;
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

std::vector<uint8_t>& Tile::getImageDataRef()
{
    return imageData_;
}

QVector<QVector3D>& Tile::getHeightVerticesRef()
{
    return heightVertices_;
}

QVector<char> &Tile::getHeightMarkVerticesRef()
{
    return heightMarkVertices_;
}

const QVector<QVector2D>& Tile::getTextureVerticesRef() const
{
    return textureVertices_;
}

const QVector<QVector3D>& Tile::getHeightVerticesConstRef() const
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
    if (qFuzzyIsNull(heightVertices_[topLeft].z()) || // someone zero
        qFuzzyIsNull(heightVertices_[topRight].z()) ||
        qFuzzyIsNull(heightVertices_[bottomLeft].z()) ||
        qFuzzyIsNull(heightVertices_[bottomRight].z())) {
        return false;
    }
    return true;
}
