#include "scene_object.h"
#include "draw_utils.h"


SceneObject::SceneObject(QObject *parent)
: QObject(parent)
, m_renderImpl(new RenderImplementation)
{}

SceneObject::SceneObject(RenderImplementation *impl, QObject *parent, QString name)
: QObject(parent)
, m_name(name)
, m_renderImpl(impl)
{}

SceneObject::SceneObject(RenderImplementation *impl, GraphicsScene3dView *view, QObject *parent, QString name)
: QObject(parent)
, m_renderImpl(impl)
, m_view(view)
{
    Q_UNUSED(name);
}

void SceneObject::mouseMoveEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)
}

void SceneObject::mousePressEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)
}

void SceneObject::mouseReleaseEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)
}

void SceneObject::mouseWheelEvent(Qt::MouseButtons buttons, qreal x, qreal y, QPointF angleDelta)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)
    Q_UNUSED(angleDelta)
}

void SceneObject::keyPressEvent(Qt::Key key)
{
    Q_UNUSED(key)
}

SceneObject::~SceneObject()
{}

void SceneObject::setName(QString name)
{
    if(m_name != name)
        m_name = name;

    Q_EMIT changed();
}

QString SceneObject::id() const
{
    return m_uuid.toString(QUuid::WithoutBraces);
}

QString SceneObject::name() const
{
    return m_name;
}

SceneObject::SceneObjectType SceneObject::type() const
{
    return SceneObjectType::Unknown;
}

QVector<QVector3D> SceneObject::data() const
{
    return m_renderImpl->data();
}

const QVector<QVector3D> &SceneObject::cdata() const
{
    return m_renderImpl->cdata();
}

bool SceneObject::isVisible() const
{
    return m_renderImpl->isVisible();
}

QColor SceneObject::color() const
{
    return m_renderImpl->color();
}

float SceneObject::width() const
{
    return m_renderImpl->width();
}

Cube SceneObject::bounds() const
{
    return m_renderImpl->bounds();
}

AbstractEntityDataFilter *SceneObject::filter() const
{
    return m_filter.get();
}

int SceneObject::primitiveType() const
{
    return m_renderImpl->primitiveType();
}

QVector3D SceneObject::position() const
{
    return m_renderImpl->bounds().center();
}

void SceneObject::setData(const QVector<QVector3D> &data, int primitiveType)
{
    m_renderImpl->setData(data, primitiveType);

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void SceneObject::setFilter(std::shared_ptr<AbstractEntityDataFilter> filter)
{
    if(m_filter == filter)
        return;

    m_filter = filter;

    Q_EMIT changed();
}

void SceneObject::removeVertex(int index)
{
    m_renderImpl->removeVertex(index);
    m_renderImpl->createBounds();

    Q_EMIT changed();
    Q_EMIT boundsChanged();
}

void SceneObject::clearData()
{
    m_renderImpl->clearData();

    Q_EMIT changed();
}

void SceneObject::setVisible(bool isVisible)
{
    m_renderImpl->setVisible(isVisible);

    Q_EMIT visibilityChanged(m_renderImpl->m_isVisible);
    Q_EMIT changed();
}

void SceneObject::setColor(QColor color)
{
    m_renderImpl->setColor(color);

    Q_EMIT changed();
}

void SceneObject::setWidth(qreal width)
{
    m_renderImpl->setWidth(width);

    Q_EMIT changed();
}

void SceneObject::qmlDeclare()
{
    qmlRegisterUncreatableType <SceneObject> ("SceneObject", 1, 0, "SceneObject", "");
    qRegisterMetaType<SceneObject::SceneObjectType>("SceneObjectType");
    qRegisterMetaType<AbstractEntityDataFilter::FilterType>("FilterType");
}

//-----------------------RenderImplementation-----------------------------//
SceneObject::RenderImplementation::RenderImplementation()
{}

SceneObject::RenderImplementation::~RenderImplementation()
{}

void SceneObject::RenderImplementation::render(QOpenGLFunctions *ctx,
                                               const QMatrix4x4 &mvp,
                                               const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
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

    ctx->glLineWidth(m_width);
    ctx->glDrawArrays(m_primitiveType, 0, m_data.size());
    ctx->glLineWidth(1.0f);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void SceneObject::RenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &model, const QMatrix4x4 &view, const QMatrix4x4 &projection, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
    render(ctx, projection * view * model, shaderProgramMap);
}

void SceneObject::RenderImplementation::setData(const QVector<QVector3D> &data, int primitiveType)
{
    if(m_primitiveType != primitiveType)
        m_primitiveType = primitiveType;

    m_data = data;

    createBounds();
}

void SceneObject::RenderImplementation::setColor(QColor color)
{
    if(m_color != color)
        m_color = color;
}

void SceneObject::RenderImplementation::setWidth(qreal width)
{
    if(m_width != width)
        m_width = width;
}

void SceneObject::RenderImplementation::setVisible(bool isVisible)
{
    if(m_isVisible != isVisible)
        m_isVisible = isVisible;
}

void SceneObject::RenderImplementation::clearData()
{
    m_data.clear();

    createBounds();
}

QVector<QVector3D> SceneObject::RenderImplementation::data() const
{
    return m_data;
}

const QVector<QVector3D> &SceneObject::RenderImplementation::cdata() const
{
    return m_data;
}

QColor SceneObject::RenderImplementation::color() const
{
    return m_color;
}

qreal SceneObject::RenderImplementation::width() const
{
    return m_width;
}

bool SceneObject::RenderImplementation::isVisible() const
{
    return m_isVisible;
}

Cube SceneObject::RenderImplementation::bounds() const
{
    return m_bounds;
}

int SceneObject::RenderImplementation::primitiveType() const
{
    return m_primitiveType;
}

void SceneObject::RenderImplementation::removeVertex(int index)
{
    if(index < 0 || index >= m_data.size())
        return;

    m_data.removeAt(index);

    createBounds();
}

void SceneObject::RenderImplementation::createBounds()
{
    if (m_data.isEmpty()) {
        m_bounds = Cube();
        return;
    }

    float z_max{ !std::isfinite(m_data.first().z()) ? 0.f : m_data.first().z() };
    float z_min{ z_max };
    float x_max{ !std::isfinite(m_data.first().x()) ? 0.f : m_data.first().x() };
    float x_min{ x_max };
    float y_max{ !std::isfinite(m_data.first().y()) ? 0.f : m_data.first().y() };
    float y_min{ y_max };

    for (const auto& itm: qAsConst(m_data)){
        z_min = std::min(z_min, !std::isfinite(itm.z()) ? 0.f : itm.z());
        z_max = std::max(z_max, !std::isfinite(itm.z()) ? 0.f : itm.z());
        x_min = std::min(x_min, !std::isfinite(itm.x()) ? 0.f : itm.x());
        x_max = std::max(x_max, !std::isfinite(itm.x()) ? 0.f : itm.x());
        y_min = std::min(y_min, !std::isfinite(itm.y()) ? 0.f : itm.y());
        y_max = std::max(y_max, !std::isfinite(itm.y()) ? 0.f : itm.y());
    }

    m_bounds = Cube(x_min, x_max, y_min, y_max, z_min, z_max);
}
