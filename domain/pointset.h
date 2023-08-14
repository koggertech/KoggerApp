#ifndef POINTSET_H
#define POINTSET_H

#include <memory>

#include <displayedobject.h>
#include <pointlistmodel.h>

class PointSet : public DisplayedObject
{
    Q_OBJECT
    Q_PROPERTY(PointListModel* pointListModel READ pointListModel NOTIFY pointListModelChanged)
    Q_PROPERTY(QVariantList pointList READ points NOTIFY dataChanged)

public:
    explicit PointSet(QObject *parent = nullptr);

    //! @brief Устанавливает набор вершин объекта.
    //! @param[in] data - ссылка на набор вершин.
    virtual void setData(const QVector <QVector3D>& data) override;

    //! @brief Добавляет вершину в конец набора вершин.
    //! @param[in] vertex - ссылка на вершину
    virtual void append(const QVector3D& vertex) override;

    //! @brief Добавляет входящий набор вершин в конец набора вершин объекта
    //! @param[in] other - ссылка на набор вершин
    virtual void append(const QVector<QVector3D>& other) override;

    virtual SceneObjectType type() const override;


    void changePointCoord(int index, QVector3D coord);

    void removePoint(int index);



private:

    QVariantList points() const;

    PointListModel *pointListModel() const;

signals:

    void pointsCountChanged();

    void pointListModelChanged();

    void pointChanged(int index);

private:

    std::shared_ptr <PointListModel> mPointListModel;
};

#endif // POINTSET_H
