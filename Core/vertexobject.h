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

#include <GL/gl.h>

#include <cube.h>
/**
 * @brief Класс объекта, представленного набором вершин
 */
class VertexObject
{
public:

    //! @brief Конструктор.
    VertexObject();

    //! @brief Конструктор с параметрами.
    //! @param[in] type - тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    VertexObject(const int type);

    //! @brief Конструктор с параметрами.
    //! @param[in] type - тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    //! @param[in] data - ссылка на набор вершин объекта.
    VertexObject(const int type, const QVector <QVector3D>& data);

    //! @brief Деструктор.
    ~VertexObject();

    /**
     * @brief Возвращает уникальный идентификатор объекта
     * @return Уникальный идентификатор объекта
     */
    QString id() const;

    //! @brief Устанавливает тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    //! @param[in] type - тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    virtual void setPrimitiveType(const int type);

    //! @brief Устанавливает набор вершин объекта.
    //! @param[in] data - ссылка на набор вершин.
    virtual void setData(const QVector <QVector3D>& data);

    //! @brief Добавляет вершину в конец набора вершин.
    //! @param[in] vertex - ссылка на вершину
    void append(const QVector3D& vertex);

    //! @brief Добавляет входящий набор вершин в конец набора вершин объекта
    //! @param[in] other - ссылка на набор вершин
    void append(const QVector<QVector3D>& other);

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

protected:

    QUuid mUuid;               //< Уникальный идентификатор объекта
    int mPrimitiveType;        //< Тип примитива для отображения в движке openGL (из набора дефайнов gl.h).
    QVector <QVector3D> mData; //< Набор вершин объекта.
    Cube mBounds = Cube(0.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f);
};
