#include "datasetchannellistmodel.h"

DatasetChannelListModel::DatasetChannelListModel(QObject *parent)
    : QStandardItemModel{parent}
{}

int DatasetChannelListModel::currentIndex()
{
    return m_currentIndex;
}

void DatasetChannelListModel::setCurrentIndex(int index)
{
    m_currentIndex = index;
}
