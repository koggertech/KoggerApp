#include "global_mesh.h"


GlobalMesh::~GlobalMesh()
{
    for (auto& itm : tiles_) {
        delete itm;
    }
}

bool GlobalMesh::concatenate(MatrixParams &actualMatParams)
{
    if (!actualMatParams.isValid()) {
        return false;
    }

    if (tileMatrix_.empty()) {
        int newNumWidthTiles = std::ceil(actualMatParams.imageWidth * 1.0f / tileSize_);
        int newNumHeightTiles = std::ceil(actualMatParams.imageHeight * 1.0f / tileSize_);

        initializeMatrix(newNumWidthTiles, newNumHeightTiles, actualMatParams);

        ++count_;
        return true;
    }

    bool resized = false;

    int columnsToAddLeft = 0, columnsToAddRight = 0, rowsToAddTop = 0, rowsToAddBottom = 0;

    if (actualMatParams.originX < origin_.x()) {
        columnsToAddLeft = std::ceil((origin_.x() - actualMatParams.originX) / tileSize_);
    }
    if ((actualMatParams.originX + actualMatParams.imageWidth) > (origin_.x() + getPixelWidth())) {
        columnsToAddRight = std::ceil(((actualMatParams.originX + actualMatParams.imageWidth) - (origin_.x() + getPixelWidth())) / tileSize_);
    }
    if ((actualMatParams.originY + actualMatParams.imageHeight) > (origin_.y() + getPixelHeight()) ) {
        rowsToAddTop = std::ceil(((actualMatParams.originY + actualMatParams.imageHeight) - (origin_.y() + getPixelHeight())) / tileSize_);

    }
    if (actualMatParams.originY < origin_.y()) {
        rowsToAddBottom = std::ceil((origin_.y() - actualMatParams.originY) / tileSize_);

    }


    if (columnsToAddLeft > 0) {
        qDebug() << "columnsToAddLeft: " << columnsToAddLeft;
        // first change origin X
        origin_.setX(origin_.x() - columnsToAddLeft * tileSize_);
        resizeColumnsLeft(columnsToAddLeft);
        resized = true;
    }
    if (rowsToAddBottom > 0) {
        qDebug() << "rowsToAddBottom: " << rowsToAddBottom;
        // first change origin Y
        origin_.setY(origin_.y() - rowsToAddBottom * tileSize_);
        resizeRowsBottom(rowsToAddBottom);
        resized = true;
    }

    if (columnsToAddRight > 0) {
        qDebug() << "columnsToAddRight: " << columnsToAddRight;

        resizeColumnsRight(columnsToAddRight);
        resized = true;
    }
    if (rowsToAddTop > 0) {
        qDebug() << "rowsToAddTop: " << rowsToAddTop;

        resizeRowsTop(rowsToAddTop);
        resized = true;
    }



    if (resized) {
        ++count_;
    }

    return resized;
}

void GlobalMesh::printMatrix() const
{
    qDebug() << "origin:" << origin_;
    qDebug() << "tiles (WxH): " << numWidthTiles_ << "x" << numHeightTiles_;


    for (size_t i = 0; i < tileMatrix_.size(); ++i) {
        QString rowOutput;
        for (size_t j = 0; j < tileMatrix_[i].size(); ++j) {
            if (tileMatrix_[i][j] != nullptr) {
                rowOutput += QString("[" + QString::number(tileMatrix_[i][j]->getSomeInt()) + "]").rightJustified(5);
            }
            else {
                rowOutput += QString("[     ]").rightJustified(5);
            }
        }
        qDebug() << rowOutput;
    }
    qDebug() << "\n";




    for (size_t i = 0; i < tileMatrix_.size(); ++i) {
        QString rowOutput;
        for (size_t j = 0; j < tileMatrix_[i].size(); ++j) {
            if (tileMatrix_[i][j] != nullptr) {
                rowOutput += QString("[" + QString::number(tileMatrix_[i][j]->tileOrigin_.x(), 'f', 0) + "x" +  QString::number(tileMatrix_[i][j]->tileOrigin_.y(), 'f', 0) + "]").rightJustified(10);
            }
            else {
                rowOutput += QString("[     ]").rightJustified(5);
            }
        }
        qDebug() << rowOutput;
    }
    qDebug() << "\n";
}

int GlobalMesh::getPixelWidth() const
{
    return numWidthTiles_ * tileSize_;
}

int GlobalMesh::getPixelHeight() const
{
    return numHeightTiles_ * tileSize_;
}

std::vector<std::vector<Tile *> > &GlobalMesh::getTileMatrixRef()
{
    return tileMatrix_;
}

void GlobalMesh::initializeMatrix(int numWidthTiles, int numHeightTiles, const MatrixParams &matrixParams)
{

    qDebug() << "first init matrix";


    numWidthTiles_ = numWidthTiles;
    numHeightTiles_ = numHeightTiles;

    origin_ = QVector3D(matrixParams.originX, matrixParams.originY, 0);

    tileMatrix_.resize(numHeightTiles_);
    for (int i = 0; i < numHeightTiles_; ++i) {
        tileMatrix_[i].resize(numWidthTiles_);
    }

    for (int i = 0; i < numHeightTiles_; ++i) {
        for (int j = 0; j < numWidthTiles_; ++j) {

            if (!tileMatrix_[i][j]) {
                tiles_.push_back(new Tile());
                tileMatrix_[i][j] = tiles_.back();


                // init tile
                tileMatrix_[i][j]->setSomeInt(count_);
                QVector3D tileOrigin = { origin_.x() + j * tileSize_, origin_.y() + (numHeightTiles - 1) * tileSize_ -  i * tileSize_, 0.0f }; // reverse in mem


                tileMatrix_[i][j]->initTile(tileOrigin, heightRatio_, tileSize_);
                tileMatrix_[i][j]->setIsUpdate(true);
            }


        }
    }
}


void GlobalMesh::resizeColumnsLeft(int columnsToAdd)
{
    for (int i = 0; i < static_cast<int>(tileMatrix_.size()); ++i) {

        int oldSize = tileMatrix_[i].size(); // width size

        tileMatrix_[i].resize(oldSize + columnsToAdd);

        for (int j = oldSize - 1; j >= 0; --j) { // перемещаем старое в новые ячейки
            tileMatrix_[i][j + columnsToAdd] = tileMatrix_[i][j];
        }

        for (int j = 0; j < columnsToAdd; ++j) { // инитим вставленное
            tiles_.push_back(new Tile());
            tileMatrix_[i][j] = tiles_.back();

            // init tile
            tileMatrix_[i][j]->setSomeInt(count_);

            QVector3D tileOrigin = { origin_.x() + j * tileSize_,   origin_.y() + (numHeightTiles_ - 1) * tileSize_ - i * tileSize_, 0.0f };

            tileMatrix_[i][j]->initTile(tileOrigin, heightRatio_, tileSize_);
            tileMatrix_[i][j]->setIsUpdate(true);

        }
    }

    numWidthTiles_ += columnsToAdd;
}

void GlobalMesh::resizeRowsBottom(int rowsToAdd)
{
    int oldHeight = tileMatrix_.size();
    tileMatrix_.resize(oldHeight + rowsToAdd);

    for (int i = oldHeight; i < static_cast<int>(tileMatrix_.size()); ++i) { // просто по новым идём сверху вниз
        tileMatrix_[i].resize(numWidthTiles_);



        for (int j = 0; j < numWidthTiles_; ++j) {
            tiles_.push_back(new Tile());
            tileMatrix_[i][j] = tiles_.back();


            // init tile
            tileMatrix_[i][j]->setSomeInt(count_);


            // т.к. ориджин обновился мы плюсуем по y
            QVector3D tileOrigin = { origin_.x() + j * tileSize_, origin_.y() + i * tileSize_, 0.0f };



            tileMatrix_[i][j]->initTile(tileOrigin, heightRatio_, tileSize_);
            tileMatrix_[i][j]->setIsUpdate(true);

        }
    }

    numHeightTiles_ += rowsToAdd;
}







void GlobalMesh::resizeColumnsRight(int columnsToAdd)
{
    int oldNumWidthTiles = numWidthTiles_;


    for (int i = 0; i < static_cast<int>(tileMatrix_.size()); ++i) { // сверху вниз


        tileMatrix_[i].resize(oldNumWidthTiles + columnsToAdd); // ресайзаем строку

        for (int j = oldNumWidthTiles; j < oldNumWidthTiles + columnsToAdd; ++j) { // просто по новым идём
            tiles_.push_back(new Tile());
            tileMatrix_[i][j] = tiles_.back();

            // init tile
            tileMatrix_[i][j]->setSomeInt(count_);

            QVector3D tileOrigin = { origin_.x() + j * tileSize_, origin_.y() + (numHeightTiles_ - 1) * tileSize_ - i * tileSize_, 0.0f };
            tileMatrix_[i][j]->initTile(tileOrigin, heightRatio_, tileSize_);
            tileMatrix_[i][j]->setIsUpdate(true);

        }
    }


    numWidthTiles_ += columnsToAdd;
}




void GlobalMesh::resizeRowsTop(int rowsToAdd)
{
    tileMatrix_.resize(numHeightTiles_ + rowsToAdd); // by height

    for (int i = tileMatrix_.size() - 1; i >= rowsToAdd; --i) { // смещаем старое
        tileMatrix_[i] = tileMatrix_[i - rowsToAdd];
    }

    for (int i = 0; i < rowsToAdd; ++i) { // инитим новое сверху вниз

        tileMatrix_[i].resize(numWidthTiles_);


        for (int j = 0; j < numWidthTiles_; ++j) {
            tiles_.push_back(new Tile());
            tileMatrix_[i][j] = tiles_.back();

            // init tile
            tileMatrix_[i][j]->setSomeInt(count_);
            QVector3D tileOrigin = { origin_.x() + j * tileSize_, origin_.y() + (numHeightTiles_ + rowsToAdd - 1) * tileSize_ -  i * tileSize_, 0.0f };

            tileMatrix_[i][j]->initTile(tileOrigin, heightRatio_, tileSize_);
            tileMatrix_[i][j]->setIsUpdate(true);
        }
    }



    numHeightTiles_ += rowsToAdd;
}

