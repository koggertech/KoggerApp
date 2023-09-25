#ifndef SCENEGRAPHICSOBJECT_H
#define SCENEGRAPHICSOBJECT_H

#include <sceneobject.h>

class SceneGraphicsObject : public SceneObject
{
    Q_OBJECT
    Q_PROPERTY(bool   visible  READ isVisible  WRITE setVisible)
    Q_PROPERTY(bool   selected READ isSelected WRITE setSelected)
    Q_PROPERTY(QColor color    READ color      CONSTANT)
    Q_PROPERTY(qreal  width    READ width      CONSTANT)

public:
    explicit SceneGraphicsObject(QObject *parent = nullptr);
    virtual ~SceneGraphicsObject();

    virtual void draw(QOpenGLFunctions* ctx,
                      const QMatrix4x4& mvp,
                      const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const;

    bool isVisible() const;
    bool isSelected() const;
    QColor color() const;
    qreal width() const;

public Q_SLOTS:
    void setVisible(bool isVisible);
    void setSelected(bool selected);
    void setColor(QColor color);
    void setWidth(qreal width);

protected:
    bool m_isSelected = false;
    bool m_isVisible = true;
    QColor m_color = QColor(255.0f, 255.0f, 255.0f);
    qreal m_width = 1.0f;
};

#endif // SCENEGRAPHICSOBJECT_H
