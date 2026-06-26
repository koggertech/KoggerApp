#pragma once

#include "scene_object.h"

#include <QColor>


class NavigationArrow : public SceneObject
{
    Q_OBJECT
public:
    enum class Shape {
        Arrow = 0,
        Boat  = 1
    };

    class NavigationArrowRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        NavigationArrowRenderImplementation();
        ~NavigationArrowRenderImplementation() override;
        void render(QOpenGLFunctions* ctx,
                    const QMatrix4x4& mvp,
                    const QMap<QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
        QVector3D getPosition() const;
        float getAngle() const;
        int getSize() const;

    private:
        friend class NavigationArrow;

        QVector3D position_;
        float angle_ = 0.0f;
        int size_ = 1;
        Shape shape_ = Shape::Arrow;

        QVector<QVector3D> arrowVertices_;
        QVector<QVector3D> arrowNormals_;
        QVector<QVector3D> arrowRibs_;
        QColor arrowFillColor_ = QColor(235, 52, 52);
        QColor arrowRibColor_  = QColor(99, 22, 22);

        QVector<QVector3D> boatVertices_;
        QVector<QVector3D> boatNormals_;
        QVector<QVector3D> boatRibs_;
        QColor boatFillColor_ = QColor(196, 114, 56);
        QColor boatRibColor_  = QColor(96, 54, 28);
    };

    explicit NavigationArrow(QObject *parent = nullptr);
    void setPositionAndAngle(const QVector3D& position, float degAngle);
    void resetPositionAndAngle();
    void setSize(int size);
    void setShape(int shape);
    void setIsDatasetExist(bool state);

private:
    /*methods*/
    QVector<QVector3D> makeArrowVertices() const;
    QVector<QVector3D> makeArrowNormals(const QVector<QVector3D>& tris) const;
    QVector<QVector3D> makeArrowRibs() const;
    QVector<QVector<QVector3D>> buildBoatStations() const;
    QVector<QVector3D> makeBoatVertices() const;
    QVector<QVector3D> makeBoatNormals(const QVector<QVector3D>& tris) const;
    QVector<QVector3D> makeBoatRibs() const;
};
