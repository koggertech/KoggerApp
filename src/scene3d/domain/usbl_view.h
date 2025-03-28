#pragma once

#include "scene_object.h"
#include <memory>
#include <QImage>
#include <QOpenGLShaderProgram>
#include <QVector>
#include <QVector3D>
#include <QColor>
#include <QMap>


class UsblView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(UsblView)

public:
    enum class UsblObjectType {
        kUndefined = 0,
        kUsbl,
        kBeacon
    };

    struct UsblObjectParams
    {
        UsblObjectParams() : isTrackVisible_(true), type_(UsblObjectType::kUndefined), lineWidth_(3.0f), pointRadius_(30.0f), objectColor_(255,55,55), data_() {};
        UsblObjectParams(bool isTrackVisisble, UsblObjectType type, float lineWidth, float pointRadius, QColor objectColor, QVector<QVector3D> data) :
            isTrackVisible_(isTrackVisisble), type_(type), lineWidth_(lineWidth), pointRadius_(pointRadius), objectColor_(objectColor), data_(data) {};

        bool isTrackVisible_;
        UsblObjectType type_;
        float lineWidth_;
        float pointRadius_;
        QColor objectColor_;
        QVector<QVector3D> data_;
    };

    class UsblViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override final;
    private:
        virtual void createBounds() override final;
        friend class UsblView;
        // data
        QMap<int, UsblObjectParams> tracks_;
    };

    explicit UsblView(QObject* parent = nullptr);
    virtual ~UsblView();

    /*SceneObject*/
    virtual SceneObjectType type() const override;

    /*UsblView*/
    void setTrackRef(QMap<int, UsblObjectParams>& tracks); // first - id, second - tracks
    void clearTracks();

private:
    // data
    QMap<int, UsblObjectParams> tracks_;
};
