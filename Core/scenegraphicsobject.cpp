#include "scenegraphicsobject.h"

#include <drawutils.h>

SceneGraphicsObject::SceneGraphicsObject(QObject *parent)
: SceneObject(parent)
{}

SceneGraphicsObject::~SceneGraphicsObject()
{}

void SceneGraphicsObject::draw(QOpenGLFunctions *ctx,
                               const QMatrix4x4 &mvp,
                               const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const
{
    if(!m_isVisible)
        return;

    if(!shaderProgramMap.contains("static"))
        return;

    auto shaderProgram = shaderProgramMap["static"];

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int colorLoc  = shaderProgram->uniformLocation("color");
    int matrixLoc = shaderProgram->uniformLocation("matrix");

    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(m_color));
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_data.constData());

    ctx->glDrawArrays(m_primitiveType, 0, m_data.count());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

bool SceneGraphicsObject::isVisible() const
{
    return m_isVisible;
}

bool SceneGraphicsObject::isSelected() const
{
    return m_isSelected;
}

QColor SceneGraphicsObject::color() const
{
    return m_color;
}

qreal SceneGraphicsObject::width() const
{
    return m_width;
}

void SceneGraphicsObject::setSelected(bool selected)
{
    if(m_isSelected == selected)
        return;

    m_isSelected = selected;
}

void SceneGraphicsObject::setColor(QColor color)
{
    if(m_color == color)
        return;

    m_color = color;
}

void SceneGraphicsObject::setWidth(qreal width)
{
    if(m_width == width)
        return;

    m_width = width;
}

void SceneGraphicsObject::setVisible(bool isVisible)
{
    if(m_isVisible == isVisible)
        return;

    m_isVisible = isVisible;
}
