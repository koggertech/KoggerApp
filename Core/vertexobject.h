//###################################################################
//! Kogger
//!
//! @author Харламенко И.В.
//! @date 2023
//###################################################################

#pragma once

#include <QVector>
#include <QVector3D>

#include <GL/gl.h>

class Cube{

public:
    Cube(float x_1, float x_2,
             float y_1, float y_2,
             float z_1, float z_2)
    :mXMin(x_1), mXMax(x_2)
    ,mYMin(y_1), mYMax(y_2)
    ,mZMin(z_1), mZMax(z_2)
    {}

    float minimumX() const {return mXMin;}
    float maximumX() const {return mXMax;}
    float minimumY() const {return mYMin;}
    float maximumY() const {return mYMax;}
    float minimumZ() const {return mZMin;}
    float maximumZ() const {return mZMax;}

    float length() const{
        return std::abs(mXMax - mXMin);
    }

    float width() const{
        return std::abs(mYMax - mYMin);
    }

    float height() const{
        return std::abs(mZMax - mZMin);
    }


private:
    float mXMin = 0.0f;
    float mXMax = 0.0f;
    float mYMin = 0.0f;
    float mYMax = 0.0f;
    float mZMin = 0.0f;
    float mZMax = 0.0f;
};

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

    /**
     * @brief Возвращает минимальное значение вершины по оси x.
     * @return Минимальное значение вершины по оси x.
     */
    float maximumX() const;

    /**
     * @brief Возвращает минимальное значение вершины по оси x.
     * @return Минимальное значение вершины по оси x.
     */
    float minimumX() const;

    /**
     * @brief Возвращает минимальное значение вершины по оси y.
     * @return Минимальное значение вершины по оси y.
     */
    float maximumY() const;

    /**
     * @brief Возвращает минимальное значение вершины по оси y.
     * @return Минимальное значение вершины по оси y.
     */
    float minimumY() const;

    Cube bounds() const;

protected:

    void createBounds();

protected:

    int mPrimitiveType;        //< Тип примитива для отображения в движке openGL (из набора дефайнов gl.h).
    QVector <QVector3D> mData; //< Набор вершин объекта.
    Cube mBounds = Cube(0.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f);
    float mMaximumZ = 0.0f;
    float mMinimumZ = 0.0f;;
    float mMaximumX = 0.0f;
    float mMinimumX = 0.0f;;
    float mMaximumY = 0.0f;
    float mMinimumY = 0.0f;;

};
