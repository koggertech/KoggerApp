#ifndef SCENEBOUNDARY_H
#define SCENEBOUNDARY_H

#include <displayedobject.h>

class SceneBoundary : public DisplayedObject
{
public:
    SceneBoundary();

    void setBoundary(const Cube& boundary);

private:

    Cube mBoundary;
};

#endif // SCENEBOUNDARY_H
