#ifndef STREAMLISTMODEL_H
#define STREAMLISTMODEL_H

#include <QAbstractListModel>

class StreamListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    StreamListModel(QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        Q_UNUSED(parent)
        return _size;
    }

    enum Roles {
        Visibility,
        ID,
        Size,
        Time,
        RecordState,
        UploadingState
    };

    void clear() {
        beginResetModel();
        _vectors.clear();
        _size = 0;
        endResetModel();
    }

signals:
    void appendEvent(int id, uint32_t size, const QString& time, int recordState, int uploadState);

private:
    Q_DISABLE_COPY(StreamListModel)

    int _size = 0;
    int _categories = 0;

    QVector<int> _roles;
    QHash<int, QByteArray> _roleNames {
        {{StreamListModel::Visibility}, {"visibity"}},
        {{StreamListModel::ID}, {"id"}},
        {{StreamListModel::Size}, {"size"}},
        {{StreamListModel::Time}, {"time"}},
        {{StreamListModel::RecordState}, {"recordState"}},
        {{StreamListModel::UploadingState}, {"uploadState"}},
    };
    QHash<int, QVector<QVariant>> _vectors;


    void doAppend(int id, uint32_t size, const QString& time, int recordState, int uploadState);
};

#endif // STREAMLISTMODEL_H
