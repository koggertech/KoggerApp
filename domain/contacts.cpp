#include "contacts.h"


Contacts::Contacts(QObject *parent) :
    SceneObject(new ContactsRenderImplementation, parent)
{

}

Contacts::~Contacts()
{

}

void Contacts::clear()
{

    auto renderImpl = RENDER_IMPL(Contacts);

    //

    renderImpl->createBounds();


    Q_EMIT changed();
    Q_EMIT boundsChanged();
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// ContactsRenderImplementation
Contacts::ContactsRenderImplementation::ContactsRenderImplementation()
{

}

void Contacts::ContactsRenderImplementation::render(QOpenGLFunctions *ctx,
                                                    const QMatrix4x4 &model,
                                                    const QMatrix4x4 &view,
                                                    const QMatrix4x4 &projection,
                                                    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible)
        return;

}
