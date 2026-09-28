#pragma once

#include <cmath>

#include <QColor>
#include <QVector>
#include <QVector3D>

#include "scene_object.h"


class UsblLayer : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(UsblLayer)

public:
    struct Beacon
    {
        int addr{-1};
        QColor color{200, 60, 60};
        bool active{true};
        QVector<QVector3D> track;
        QVector3D surface;
        QVector3D deep;
        bool hasFix{false};
        bool hasDeep{false};
    };

    struct Head
    {
        bool hasFix{false};
        QVector<QVector3D> track;
        QVector3D pos;
        float yawDeg{NAN};
        bool hasYaw{false};
    };

    // A track is either the path travelled or the places a fix actually landed. The line reads
    // as motion and hides how sparse the fixes are; the dots read as measurements and hide the
    // order they came in. Neither is a superset, so both are offered.
    enum class TrackStyle
    {
        Line = 0,
        Dots = 1
    };

    struct RenderData
    {
        QVector<Beacon> beacons;
        Head head;
    };

    class UsblLayerRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        void setRenderData(RenderData data);

        // The four-matrix overload, not the mvp one: the address labels are 2D text placed at the
        // screen projection of a 3D point, which needs view and projection apart.
        void render(QOpenGLFunctions* ctx, const QMatrix4x4& model, const QMatrix4x4& view,
                    const QMatrix4x4& projection,
                    const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const final;

    private:
        void updateBounds() final;
        void drawBall(QOpenGLFunctions* ctx, QOpenGLShaderProgram* prog, int posLoc, int colorLoc,
                      int widthLoc, const QVector3D& at, const QColor& fill, float radiusPx,
                      float opacity) const;

        friend class UsblLayer;
        RenderData data_;
        TrackStyle beaconTrackStyle_{TrackStyle::Line};
        TrackStyle headTrackStyle_{TrackStyle::Line};
    };

    explicit UsblLayer(QObject* parent = nullptr);
    ~UsblLayer() override;

    /*SceneObject*/
    SceneObjectType type() const override;

    /*UsblLayer*/
    void setRenderData(RenderData data);
    void setBeaconTrackStyle(TrackStyle style);
    void setHeadTrackStyle(TrackStyle style);
    void clear();
};
