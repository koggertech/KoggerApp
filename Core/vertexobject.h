//###################################################################
//! Kogger
//!
//! @author Харламенко И.В.
//! @date 2023
//###################################################################

#pragma once

#include <QVector>
#include <QVector3D>

#if !defined(Q_OS_ANDROID)
#include <GL/gl.h>
#else
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#endif
/**
 * @brief Класс объекта, представленного набором вершин
 */
class VertexObject
{
public:

    //! @brief Конструктор.
    VertexObject() = default;

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
     * @brief Возвращает максимальное значение вершины по оси z.
     * @return Максимальное значение вершины по оси z.
     */
    float maximumZ() const;

    /**
     * @brief Возвращает минимальное значение вершины по оси z.
     * @return Минимальное значение вершины по оси z.
     */
    float minimumZ() const;

protected:

    int mPrimitiveType;        //< Тип примитива для отображения в движке openGL (из набора дефайнов gl.h).
    QVector <QVector3D> mData; //< Набор вершин объекта.
    float mMaximumZ = 0.0f;
    float mMinimumZ = 0.0f;;

};
