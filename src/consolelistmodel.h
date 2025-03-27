#ifndef CONSOLELISTMODEL_H
#define CONSOLELISTMODEL_H

#include <QAbstractListModel>
#include <math.h>

class ConsoleListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ConsoleListModel(QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        Q_UNUSED(parent)
        return _size;
    }

    void init();

    enum Roles {
        Visibility,
        Time,
        Category,
        Payload,
    };

signals:
    void appendEvent(const QString& time, int category, const QString& data);

private:
    Q_DISABLE_COPY(ConsoleListModel)

    int _size = 0;
    int _categories = 0;

    QVector<int> _roles;
    QHash<int, QByteArray> _roleNames {
        {{ConsoleListModel::Visibility}, {"visibity"}},
        {{ConsoleListModel::Time}, {"time"}},
        {{ConsoleListModel::Category}, {"category"}},
        {{ConsoleListModel::Payload}, {"payload"}},
    };
    QHash<int, QVector<QVariant>> _vectors;


    void doAppend(const QString& time, int category, const QString& data);
};

#endif // CONSOLELISTMODEL_H
