#include "global_mesh.h"


GlobalMesh::~GlobalMesh()
{
    for (auto& itm : tiles_) {
        delete itm;
    }
}

bool GlobalMesh::concatenate(MatrixParams &actualMatParams) // work with unscaled data (heightMatrix)
{
    if (!actualMatParams.isValid()) {
        return false;
    }

    //int tileSizeInPixel = static_cast<int>(tileSizeMeters_ * 1.0f / resolution);

    if (tileMatrix_.empty()) {
        int newNumWidthTiles = std::ceil(actualMatParams.heightMatrixWidth * 1.0f / tileSizeMeters_);
        int newNumHeightTiles = std::ceil(actualMatParams.heightMatrixHeight * 1.0f / tileSizeMeters_);

        initializeMatrix(newNumWidthTiles, newNumHeightTiles, actualMatParams);

        ++count_;
        return true;
    }

    bool resized = false;

    int columnsToAddLeft = 0, columnsToAddRight = 0, rowsToAddTop = 0, rowsToAddBottom = 0;
    if (actualMatParams.originX < origin_.x()) {
        columnsToAddLeft = std::ceil((origin_.x() - actualMatParams.originX) / tileSizeMeters_);
    }
    if ((actualMatParams.originX + actualMatParams.heightMatrixWidth) > (origin_.x() + getWidthMeters())) {
        columnsToAddRight = std::ceil(((actualMatParams.originX + actualMatParams.heightMatrixWidth) - (origin_.x() + getWidthMeters())) / tileSizeMeters_);
    }
    if ((actualMatParams.originY + actualMatParams.heightMatrixHeight) > (origin_.y() + getHeightMeters()) ) {
        rowsToAddTop = std::ceil(((actualMatParams.originY + actualMatParams.heightMatrixHeight) - (origin_.y() + getHeightMeters())) / tileSizeMeters_);
    }
    if (actualMatParams.originY < origin_.y()) {
        rowsToAddBottom = std::ceil((origin_.y() - actualMatParams.originY) / tileSizeMeters_);
    }


    if (columnsToAddLeft > 0) {
        // first change origin X
        origin_.setX(origin_.x() - columnsToAddLeft * tileSizeMeters_);
        resizeColumnsLeft(columnsToAddLeft);
        resized = true;
    }
    if (rowsToAddBottom > 0) {
        // first change origin Y
        origin_.setY(origin_.y() - rowsToAddBottom * tileSizeMeters_);
        resizeRowsBottom(rowsToAddBottom);
        resized = true;
    }

    if (columnsToAddRight > 0) {
        resizeColumnsRight(columnsToAddRight);
        resized = true;
    }
    if (rowsToAddTop > 0) {
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
                auto tileOrigin = tileMatrix_[i][j]->getTileOrigin();
                rowOutput += QString("[" + QString::number(tileOrigin.x(), 'f', 0) + "x" +  QString::number(tileOrigin.y(), 'f', 0) + "]").rightJustified(10);
            }
            else {
                rowOutput += QString("[     ]").rightJustified(5);
            }
        }
        qDebug() << rowOutput;
    }
    qDebug() << "\n";
}

int GlobalMesh::getWidthMeters() const
{
    return numWidthTiles_ * tileSizeMeters_;
}

int GlobalMesh::getHeightMeters() const
{
    return numHeightTiles_ * tileSizeMeters_;
}

int GlobalMesh::getNumWidthTiles() const
{
    return numWidthTiles_;
}

int GlobalMesh::getNumHeightTiles() const
{
    return numHeightTiles_;
}

std::vector<std::vector<Tile *> > &GlobalMesh::getTileMatrixRef()
{
    return tileMatrix_;
}

int GlobalMesh::getTileSize() const
{
    return tileSizeMeters_;
}

int GlobalMesh::getHeightVerticeRatio() const
{
    return heightVerticeRatio_;
}

int GlobalMesh::getHeightStep() const
{
    return tileSizeMeters_ / heightVerticeRatio_;
}

QVector3D GlobalMesh::getOrigin() const
{
    return origin_;
}


void GlobalMesh::clear()
{
    origin_ = QVector3D();
    for (auto& itm: tiles_)
    {
        delete itm;
    }
    tiles_.clear();

    tileMatrix_.clear();

    numWidthTiles_ = 0;
    numHeightTiles_ = 0;

    count_ = 0;
}

void GlobalMesh::initializeMatrix(int numWidthTiles, int numHeightTiles, const MatrixParams &matrixParams)
{
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
                QVector3D tileOrigin = { origin_.x() + j * tileSizeMeters_, origin_.y() + ((numHeightTiles_ - 1) - i) * tileSizeMeters_, 0.0f }; // reverse in mem
                tileMatrix_[i][j]->initTile(tileOrigin, heightVerticeRatio_, tileSizeMeters_);
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
            QVector3D tileOrigin = { origin_.x() + j * tileSizeMeters_,   origin_.y() + ((numHeightTiles_ - 1) - i) * tileSizeMeters_, 0.0f };
            tileMatrix_[i][j]->initTile(tileOrigin, heightVerticeRatio_, tileSizeMeters_);
            tileMatrix_[i][j]->setIsUpdate(true);
        }
    }

    numWidthTiles_ += columnsToAdd;
}

void GlobalMesh::resizeRowsBottom(int rowsToAdd)
{
    int oldHeight = tileMatrix_.size();
    tileMatrix_.resize(oldHeight + rowsToAdd);

    int cnt = 0;
    for (int i = oldHeight; i < static_cast<int>(tileMatrix_.size()); ++i) { // просто по новым идём сверху вниз
        tileMatrix_[i].resize(numWidthTiles_);

        qDebug() << "used i:" << i;
        qDebug () << "origin_.y(): " << origin_.y();
        qDebug() << "i * tilesize:" << i * tileSizeMeters_;


        for (int j = 0; j < numWidthTiles_; ++j) {
            tiles_.push_back(new Tile());
            tileMatrix_[i][j] = tiles_.back();



            // init tile
            tileMatrix_[i][j]->setSomeInt(count_);            
            QVector3D tileOrigin = { origin_.x() + j * tileSizeMeters_, origin_.y() + cnt * tileSizeMeters_, 0.0f }; // т.к. ориджин обновился мы плюсуем по y
            tileMatrix_[i][j]->initTile(tileOrigin, heightVerticeRatio_, tileSizeMeters_);
            tileMatrix_[i][j]->setIsUpdate(true);
        }
        cnt++;
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
            QVector3D tileOrigin = { origin_.x() + j * tileSizeMeters_, origin_.y() + ((numHeightTiles_ - 1) - i) * tileSizeMeters_, 0.0f };
            tileMatrix_[i][j]->initTile(tileOrigin, heightVerticeRatio_, tileSizeMeters_);
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
            QVector3D tileOrigin = { origin_.x() + j * tileSizeMeters_, origin_.y() + ((numHeightTiles_ + rowsToAdd - 1) - i) * tileSizeMeters_, 0.0f };
            tileMatrix_[i][j]->initTile(tileOrigin, heightVerticeRatio_, tileSizeMeters_);
            tileMatrix_[i][j]->setIsUpdate(true);
        }
    }

    numHeightTiles_ += rowsToAdd;
}

