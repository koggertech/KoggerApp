#ifndef TST_BASIC_H
#define TST_BASIC_H

#include <QtTest>

#include "tinsplitsmoothsurfaceprocessor.hpp"
#include "gridgenerator.h"

class TestBasic : public QObject
{
    Q_OBJECT

public:
    TestBasic();

private:

private Q_SLOTS:
    void initTestCase();

    void generateQuadGridTestCase();
    void generateZeroSizeQuadGridTestCase();
    void generateZeroCellSizeQuadGridTestCase();

    void cleanupTestCase();
};

#endif // TST_BASIC_H
