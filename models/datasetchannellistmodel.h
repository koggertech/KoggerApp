#ifndef DATASETCHANNELLISTMODEL_H
#define DATASETCHANNELLISTMODEL_H

#include <QStandardItemModel>

class DatasetChannelListModel : public QStandardItemModel
{
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)

public:
    explicit DatasetChannelListModel(QObject *parent = nullptr);

    int currentIndex();
    void setCurrentIndex(int index);

private:
    int m_currentIndex = -1;
};

#endif // DATASETCHANNELLISTMODEL_H
