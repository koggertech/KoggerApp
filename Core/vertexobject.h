//###################################################################
//! Kogger
//!
//! @author Харламенко И.В.
//! @date 2023
//###################################################################

#pragma once

#include <QVector>
#include <QVector3D>
#include <QUuid>
#include <QVariantMap>

#include <GL/gl.h>

#include "sceneobject.h"
#include <cube.h>

/**
 * @brief Класс объекта, представленного набором вершин
 */
class VertexObject : public SceneObject
{
    Q_OBJECT

    Q_PROPERTY(int primitiveType READ primitiveType CONSTANT)

public:
    enum PrimitiveType {
        Point,
        Line,
        Triangle,
        Quad,
        Polygon
    };

    Q_ENUM(PrimitiveType)

    //! @brief Конструктор с параметрами.
    //! @param[in] type - тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    VertexObject(const int type, QObject* parent = nullptr);

    //! @brief Конструктор с параметрами.
    //! @param[in] type - тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    //! @param[in] data - ссылка на набор вершин объекта.
    VertexObject(const int type, const QVector <QVector3D>& data, QObject* parent = nullptr);

    //! @brief Деструктор.
    virtual ~VertexObject();

    //! @brief Устанавливает тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    //! @param[in] type - тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    virtual void setPrimitiveType(int primitiveType);

    //! @brief Устанавливает набор вершин объекта.
    //! @param[in] data - ссылка на набор вершин.
    virtual void setData(const QVector <QVector3D>& data);

    //! @brief Добавляет вершину в конец набора вершин.
    //! @param[in] vertex - ссылка на вершину
    virtual void append(const QVector3D& vertex);

    //! @brief Добавляет входящий набор вершин в конец набора вершин объекта
    //! @param[in] other - ссылка на набор вершин
    virtual void append(const QVector<QVector3D>& other);

    virtual void remove(int index);

    //! @brief Возвращает тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    //! @return[in] Тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    int primitiveType() const;

    //! @brief Возвращает копию набора вершин объекта.
    //! @return Копмя набора вершин объекта.
    QVector <QVector3D> data() const;

    //! @brief Возвращает константную ссылку на набор вершин объекта.
    //! @return Константная ссылка на набор вершин объекта.
    const QVector <QVector3D>& cdata() const;

    /**
     * @brief Возвращает границы объекта в виде куба.
     * @return Границы объекта в виде куба.
     */
    Cube bounds() const;

    /**
     * @brief Удаляет данные вершинного объекта.
     */
    void clearData();

protected:
    void createBounds();

signals:

    void primitiveTypeChanged(int primitiveType);

    void dataChanged();

    void boundsChanged();

protected:

    QUuid mUuid        = QUuid::createUuid(); ///< Уникальный идентификатор объекта
    int mPrimitiveType = GL_POINTS;           ///< Тип примитива для отображения в движке openGL (из набора дефайнов gl.h).
    QVector <QVector3D> mData;                ///< Набор вершин объекта.
    Cube mBounds;                             ///< Bounding cube of the object
};
