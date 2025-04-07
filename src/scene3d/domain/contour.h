#ifndef CONTOUR_H
#define CONTOUR_H

#include "scene_object.h"

class Contour : public SceneObject
{
    Q_OBJECT

public:
    explicit Contour(QObject* parent = nullptr);
    virtual ~Contour();
    bool keyPointsVisible() const;
    QColor keyPointsRgbColor() const;
    QVector4D keyPointsColor() const;

public Q_SLOTS:
    void setKeyPointsVisible(bool visible);
    void setKeyPointsColor(QColor color);

protected:
    bool   m_keyPointsVisible = false;
    QColor m_keyPointsColor = QColor(255.0f, 255.0f, 255.0f, 255.0f);

private:
    friend class Surface;
};

#endif // CONTOUR_H
