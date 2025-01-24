#pragma once

#include "sceneobject.h"

#include <QEvent>
#include "plotcash.h"


class Contacts : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Contacts)

public:
    struct ContactInfo {
        QString info;
        float lat = 0.0f;
        float lon = 0.0f;
        QVector3D nedPos;
    };

    /*structures*/
    class ContactsRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        ContactsRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& model,
                            const QMatrix4x4& view,
                            const QMatrix4x4& projection,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override final;

        void clear();

        /*data*/
        QHash<int, QRectF> contactBounds_;

    private:
        friend class Contacts;
        /*data*/
        QMap<int, ContactInfo> points_; // first - epoch index, second - position and text
        int intersectedEpochIndx_;
    };

    /*methods*/
    explicit Contacts(QObject* parent = nullptr);
    virtual ~Contacts();

    void clear();
    void setDatasetPtr(Dataset* datasetPtr);

    /*QObject*/
    virtual bool eventFilter(QObject *watched, QEvent *event) override final;

public slots:

signals:

protected:
    friend class GraphicsScene3dView;

    void mouseMoveEvent(Qt::MouseButtons buttons, qreal x, qreal y) override final;
    void mousePressEvent(Qt::MouseButtons buttons, qreal x, qreal y) override final;
    void mouseReleaseEvent(Qt::MouseButtons buttons, qreal x, qreal y) override final;
    void mouseWheelEvent(Qt::MouseButtons buttons, qreal x, qreal y, QPointF angleDelta) override final;
    void keyPressEvent(Qt::Key key) override final;

private:
    void setInterEpIndx(int indx);

    /*data*/
    QHash<int, QRectF> contactBounds_;
    Dataset* datasetPtr_;

};
