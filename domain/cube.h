#ifndef CUBE_H
#define CUBE_H

#include <cmath>

class Cube{

public:

    Cube()
    :mXMin(0.0f), mXMax(0.0f)
    ,mYMin(0.0f), mYMax(0.0f)
    ,mZMin(0.0f), mZMax(0.0f)
    {}

    Cube(float x_1, float x_2,
             float y_1, float y_2,
             float z_1, float z_2)
    :mXMin(x_1), mXMax(x_2)
    ,mYMin(y_1), mYMax(y_2)
    ,mZMin(z_1), mZMax(z_2)
    {}

    float minimumX() const {return mXMin;}
    float maximumX() const {return mXMax;}
    float minimumY() const {return mYMin;}
    float maximumY() const {return mYMax;}
    float minimumZ() const {return mZMin;}
    float maximumZ() const {return mZMax;}

    float length() const{
        return std::abs(mXMax - mXMin);
    }

    float width() const{
        return std::abs(mYMax - mYMin);
    }

    float height() const{
        return std::abs(mZMax - mZMin);
    }


private:
    float mXMin = 0.0f;
    float mXMax = 0.0f;
    float mYMin = 0.0f;
    float mYMax = 0.0f;
    float mZMin = 0.0f;
    float mZMax = 0.0f;
};


#endif // CUBE_H
