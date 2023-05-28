//###################################################################
//! Kogger
//!
//! @author Харламенко И.В.
//! @date 2023
//###################################################################

#pragma once

#include <vertexobject.h>

#include <QColor>
#include <QVector4D>

/**
 * @brief класс отображаемого на 3D - сцене объекта.
 */
class DisplayedObject : public VertexObject
{
public:

    /**
     * @brief Конструктор.
     */
    DisplayedObject();

    /**
     *  @brief Конструктор с параметрами.
     *  @param[in] type - тип примитива для отображения в движке openGL
     *  (из набора дефайнов gl.h).
     */
    DisplayedObject(const int type);

    /**
     *  @brief Конструктор с параметрами.
     *  @param[in] type - тип примитива для отображения в движке openGL
     *  (из набора дефайнов gl.h).
     *  @param[in] data - ссылка на набор вершин объекта.
     */
    DisplayedObject(const int type, const QVector <QVector3D>& data);

    /**
     *  @brief Деструктор.
     */
    ~DisplayedObject();

    /**
     *  @brief Устанавливает признак видимости объекта.
     *  @param[in] isVisible Признак видимости.
     */
    void setVisible(bool isVisible);

    /**
     * @brief Устанавливает тип примитива для отображения в движке openGL
     * (из набора дефайнов gl.h).
     * @param[in] type - тип примитива для отображения в движке openGL
     * (из набора дефайнов gl.h).
     */
    void setPrimitiveType(const int type) override;

    /**
     * @brief Устанавливает набор вершин объекта.
     * @param[in] data - ссылка на набор вершин.
     */
    virtual void setData(const QVector <QVector3D>& data) override;

    /**
     *  @brief Устанавливает признак видимости сетки объекта.
     *  @param[in] isVisible Признак видимости сетки объекта.
     */
    void setGridVisible(bool isGridVisible);

    /**
     *  @brief Устанавливает цвет объекта.
     *  @param[in] color Цвет объекта.
     */
    void setColor(QColor color);

    /**
     *  @brief Возвращает признак видимости объекта.
     *  @return Признак видимости объекта.
     */
    bool isVisible() const;

    /**
     *  @brief Возвращает признак видимости сетки объекта.
     *  @return Признак видимости сетки объекта.
     */
    bool isGridVisible() const;

    /**
     *  @brief Возвращает цвет объекта.
     *  @return Цвет объекта.
     */
    QColor rgbColor() const;

    /**
     *  @brief Возвращает цвет объекта.
     *  @return Цвет объекта.
     */
    QVector4D color() const;

    /**
     * @brief Возвращает копию набора вершин сетки объекта.
     * @return Копия набора вершин сетки объекта.
     */
    QVector <QVector3D> grid() const;

    /**
     * @brief Возвращает константную ссылку на набор вершин сетки объекта.
     * @return Константная ссылка на набор вершин сетки объекта.
     */
    const QVector <QVector3D>& cgrid() const;

    /**
     * @brief Устанавливает вершинный объект.
     * @param vertexObject Экземпляр вершинного объекта.
     */
    void setVertexObject(const VertexObject& vertexObject);

    /**
     * @brief Удаляет данные вершин поверхности
     */
    void clear();

private:

    void updateGrid();
    void makeTriangleGrid();
    void makeQuadGrid();

protected:

    bool mIsOverlapping = false;                            //< Признак того, что объект перекрывается при отрисовке с другим объектом.
    bool mIsVisible = false;                                 //< Признак видимости объекта на 3D - сцене.
    bool mIsGridVisible = false;                            //< Признак видимости сетки объекта на 3D - сцене.
    QColor mColor = QColor(255.0f, 255.0f, 255.0f, 255.0f); //< Цвет объекта.
    QVector <QVector3D> mGrid;                              //< Набор вершин сетки объекта.

};
