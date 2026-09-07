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

    Q_INVOKABLE void setMaxRows(int rows);
    Q_INVOKABLE int maxRows() const { return _maxRows; }

    Q_INVOKABLE QString rowText(int row) const;
    Q_INVOKABLE QString rangeText(int from, int to) const;

    enum Roles : uint8_t {
        Time,
        Category,
        Payload,
    };

signals:
    void appendEvent(const QString& time, int category, const QString& data);
    void rowsTrimmed(int count);

private:
    Q_DISABLE_COPY(ConsoleListModel)

    static constexpr int kMaxRows = 4000;
    static constexpr int kMinRows = 50;
    static constexpr int kTrimBatch = 256;

    int _size = 0;
    int _maxRows = kMaxRows;

    QHash<int, QByteArray> _roleNames {
        {{ConsoleListModel::Time}, {"time"}},
        {{ConsoleListModel::Category}, {"category"}},
        {{ConsoleListModel::Payload}, {"payload"}},
    };
    QHash<int, QVector<QVariant>> _vectors;

    void removeHead(int removeCount);
    void trimHeadIfNeeded(int incomingCount = 1);
    void doAppend(const QString& time, int category, const QString& data);
};

#endif // CONSOLELISTMODEL_H
