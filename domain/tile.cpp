#include "tile.h"

#include <QDebug>


Tile::Tile() : id_(QUuid::createUuid()), someInt_(-1), textureId_(0), isUpdate_(false)
{

}

void Tile::initTile(QVector3D origin, int heightRatio, int tileSize, QImage::Format imageFormat)
{
    QString imagePath = "C:/Users/salty/Desktop/1.png";
    if (!image_.load(imagePath)) {
        qWarning() << "Failed to load image from" << imagePath;
    }

    // height mat
    int heightMatSideSize = tileSize / heightRatio + 1;
    int heightVerticesSize = heightMatSideSize * heightMatSideSize;
    heightVertices_.resize(heightVerticesSize);

    for (int i = 0; i < heightMatSideSize; ++i) {

        QString sads;
        for (int j = 0; j < heightMatSideSize; ++j) {
            float x = origin.x() + j * heightRatio;
            float y = origin.y() + i * heightRatio;

            heightVertices_[i * heightMatSideSize + j] = QVector3D(x, y, -4.0f);

            sads += "[" + QString::number(x,'f',0) + "x" + QString::number(y,'f',0) + "] ";
        }
        //qDebug() << sads;
    }



    for (int i = 0; i < heightMatSideSize; ++i) {
        for (int j = 0; j < heightMatSideSize; ++j) {
            textureVertices_.append(QVector2D(float(j) / (heightMatSideSize - 1), float(i) / (heightMatSideSize - 1)));
        }
    }

    for (int i = 0; i < heightMatSideSize - 1; ++i) { // -1 для норм прохода
        for (int j = 0; j < heightMatSideSize - 1; ++j) {
            //textureVertices_.append( QVector2D(float(j) / (heightMatSideSize )   ,float(i) / (heightMatSideSize  )));

            int topLeft = i * heightMatSideSize + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * heightMatSideSize + j;
            int bottomRight = bottomLeft + 1;

            if (qFuzzyCompare(1.0f, 1.0f + heightVertices_[topLeft].z()) || // someone zero
                qFuzzyCompare(1.0f, 1.0f + heightVertices_[topRight].z()) ||
                qFuzzyCompare(1.0f, 1.0f + heightVertices_[bottomLeft].z()) ||
                qFuzzyCompare(1.0f, 1.0f + heightVertices_[bottomRight].z())) {
                continue;
            }

            // это для отрисовки
            heightIndices_.append(topLeft);     // 1--3
            heightIndices_.append(bottomLeft);  // | /
            heightIndices_.append(topRight);    // 2
            heightIndices_.append(topRight);    //    1
            heightIndices_.append(bottomLeft);  //  / |
            heightIndices_.append(bottomRight); // 2--3
        }
    }

    tileOrigin_ = origin;
}

void Tile::setSomeInt(int val)
{
    someInt_ = val;
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

int Tile::getSomeInt() const
{
    return someInt_;
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

QImage Tile::getImage()
{
    return image_;
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
