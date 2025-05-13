#pragma once

#include "scene_object.h"

#include <QEvent>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include "dataset.h"


class Contacts : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Contacts)

public:
    static void setQmlInstance(Contacts* instance, QQmlApplicationEngine* engine) {
        if (instance) {
            engine->rootContext()->setContextProperty("contacts", instance);
        }
    }

    Q_PROPERTY(QString contactInfo      READ getContactInfo      /*WRITE setContactInfo*/    NOTIFY contactChanged)
    Q_PROPERTY(bool    contactVisible   READ getContactVisible   /*WRITE setContactVisible*/ NOTIFY contactChanged)
    Q_PROPERTY(int     contactPositionX READ getContactPositionX                         NOTIFY contactChanged)
    Q_PROPERTY(int     contactPositionY READ getContactPositionY                         NOTIFY contactChanged)
    Q_PROPERTY(int     contactIndx      READ getContactIndx                              NOTIFY contactChanged)
    Q_PROPERTY(double  contactLat       READ getContactLat                               NOTIFY contactChanged)
    Q_PROPERTY(double  contactLon       READ getContactLon                               NOTIFY contactChanged)
    Q_PROPERTY(double  contactDepth     READ getContactDepth                             NOTIFY contactChanged)

    struct ContactInfo {
        QString info;
        float lat = 0.0f;
        float lon = 0.0f;
        float depth = 0.0f;
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

    QString getContactInfo() const;
    bool    getContactVisible() const;
    int     getContactPositionX() const;
    int     getContactPositionY() const;
    int     getContactIndx() const;
    double  getContactLat() const;
    double  getContactLon() const;
    double  getContactDepth() const;

    void setContactVisible(bool state);
    void clear();
    void setDatasetPtr(Dataset* datasetPtr);

    /*QObject*/
    virtual bool eventFilter(QObject *watched, QEvent *event) override final;

public slots:
    Q_INVOKABLE bool setContact(int indx, const QString& text);
    Q_INVOKABLE bool deleteContact(int indx);
    Q_INVOKABLE void update();

signals:
    void contactChanged();

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

    // qproperty
    bool contactVisible_ = false;
    int indx_ = -1;
    int positionX_ = -1;
    int positionY_ = -1;
    QString info_;
    double lat_ = 0.0;
    double lon_ = 0.0;
    double depth_ = 0.0;
};
