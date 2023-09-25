#ifndef BOTTOMTRACK_H
#define BOTTOMTRACK_H

#include <memory>
#include <scenegraphicsobject.h>
#include <abstractentitydatafilter.h>

class BottomTrack : public SceneGraphicsObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT("BottomTrack")

public:

    explicit BottomTrack(QObject* parent = nullptr);

    virtual ~BottomTrack();

    /**
     * @brief Returns length of the bottom track
     * @return Length of the route
     */
    float routeLength() const;

    //! @brief Устанавливает набор вершин объекта.
    //! @param[in] data - ссылка на набор вершин.
    virtual void setData(const QVector <QVector3D>& data) override;

    virtual SceneObjectType type() const override;

    virtual void draw(QOpenGLFunctions* ctx,
                      const QMatrix4x4& mvp,
                      const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
};

#endif // BOTTOMTRACK_H
