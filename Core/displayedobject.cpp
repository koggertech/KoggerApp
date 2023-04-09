#include "displayedobject.h"

DisplayedObject::DisplayedObject()
: VertexObject(GL_TRIANGLES)
{

}

DisplayedObject::DisplayedObject(const int type)
: VertexObject(type)
{
}

DisplayedObject::DisplayedObject(const int type, const QVector<QVector3D> &data)
: VertexObject(type, data)
{
    updateGrid();
}

DisplayedObject::~DisplayedObject()
{
}

void DisplayedObject::setVisible(bool isVisible)
{
    mIsVisible = isVisible;
}

void DisplayedObject::setPrimitiveType(const int type)
{
    VertexObject::setPrimitiveType(type);

    updateGrid();
}

void DisplayedObject::setData(const QVector<QVector3D> &data)
{
    VertexObject::setData(data);

    updateGrid();
}

void DisplayedObject::setGridVisible(bool isGridVisible)
{
    mIsGridVisible = isGridVisible;
}

void DisplayedObject::setColor(QColor color)
{
    mColor = color;
}

bool DisplayedObject::isVisible() const
{
    return mIsVisible;
}

bool DisplayedObject::isGridVisible() const
{
    return mIsGridVisible;
}

QColor DisplayedObject::rgbColor() const
{
    return mColor;
}

QVector4D DisplayedObject::color() const
{
    float rgb_max = 255.0f;

    auto r = mColor.red();
    auto g = mColor.green();
    auto b = mColor.blue();
    auto a = mColor.alpha();

    QVector4D vec{static_cast <float>(r) / rgb_max
                 ,static_cast <float>(g) / rgb_max
                 ,static_cast <float>(b) / rgb_max
                 ,static_cast <float>(a) / rgb_max};

    return vec;
}

QVector<QVector3D> DisplayedObject::grid() const
{
    return mGrid;
}

const QVector<QVector3D> &DisplayedObject::cgrid() const
{
    return mGrid;
}

void DisplayedObject::setVertexObject(const VertexObject &vertexObject)
{
    setPrimitiveType(vertexObject.primitiveType());
    setData(vertexObject.cdata());

    updateGrid();
}

void DisplayedObject::clear()
{
    mData.clear();
    mGrid.clear();
}

void DisplayedObject::updateGrid()
{
    switch(mPrimitiveType){
    case GL_TRIANGLES:
        makeTriangleGrid();
        break;
    case GL_QUADS:
        makeQuadGrid();
        break;
    default:
        mGrid = mData;
        break;
    }
}

void DisplayedObject::makeTriangleGrid()
{
    mGrid.clear();

    if (mData.size() < 3)
        return;

    for (int i = 0; i < mData.size()-3; i+=3){
        QVector3D A = mData[i];
        QVector3D B = mData[i+1];
        QVector3D C = mData[i+2];

        A.setZ(A.z() + 0.05);
        B.setZ(B.z() + 0.05);
        C.setZ(C.z() + 0.05);

        mGrid.append({A, B,
                      B, C,
                      A, C});
    }
}

void DisplayedObject::makeQuadGrid()
{
    mGrid.clear();

    if (mData.size() < 4)
        return;



    for (int i = 0; i < mData.size()-4; i+=4){      
        QVector3D A = mData[i];
        QVector3D B = mData[i+1];
        QVector3D C = mData[i+2];
        QVector3D D = mData[i+3];

        A.setZ(A.z() + 0.05f);
        B.setZ(B.z() + 0.05f);
        C.setZ(C.z() + 0.05f);
        D.setZ(C.z() + 0.05f);

        mGrid.append({A, B,
                      B, C,
                      C, D,
                      A, D});
    }
}
