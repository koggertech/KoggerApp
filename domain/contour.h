#ifndef CONTOUR_H
#define CONTOUR_H

#include <displayedobject.h>

class Contour : public DisplayedObject
{
public:
    Contour();

    float lineWidth() const;

    bool keyPointsVisible() const;

    QColor keyPointsRgbColor() const;

    QVector4D keyPointsColor() const;

    void setLineWidth(float width);

    void setKeyPointsVisible(bool visible);

    void setKeyPointsColor(QColor color);

protected:

    float mLineWidth = 1.0f;
    bool mKeyPointsVisible = false;
    QColor mKeyPointsColor = QColor(255.0f, 255.0f, 255.0f, 255.0f);
};

#endif // CONTOUR_H
