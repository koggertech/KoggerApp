#ifndef CONTOUR_H
#define CONTOUR_H

#include <displayedobject.h>

class Contour : public DisplayedObject
{
    Q_OBJECT

public:
    explicit Contour(QObject* parent = nullptr);

    virtual ~Contour();

    bool keyPointsVisible() const;

    QColor keyPointsRgbColor() const;

    QVector4D keyPointsColor() const;

    void setKeyPointsVisible(bool visible);

    void setKeyPointsColor(QColor color);

protected:

    bool   mKeyPointsVisible = false;
    QColor mKeyPointsColor = QColor(255.0f, 255.0f, 255.0f, 255.0f);
};

#endif // CONTOUR_H
