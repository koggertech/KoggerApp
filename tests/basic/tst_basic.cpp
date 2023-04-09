#include "tst_basic.h"

TestBasic::TestBasic()
{

}

void TestBasic::initTestCase()
{

}

void TestBasic::generateQuadGridTestCase()
{
    Point3D <float> topLeft(0.0f,0.0f,0.0f);
    int width = 10;
    int height = 20;
    float cellSideSize = 0.2f;

    auto grid = GridGenerator <float>::generateQuadGrid(topLeft, width, height, cellSideSize);

    QVERIFY(grid.size() > 4);
    QVERIFY(grid.size() % 4 == 0);
    QCOMPARE(grid.size(), static_cast <size_t>(width * height * 4));

    for (size_t i = 0; i < grid.size()-4; i+=4){
        QCOMPARE(grid[i+1].x() - grid[i].x(), cellSideSize);
        QVERIFY(grid[i+1].y() == grid[i].y());

        QCOMPARE(grid[i+2].y() - grid[i+1].y(), cellSideSize);
        QVERIFY(grid[i+2].x() == grid[i+1].x());

        QCOMPARE(grid[i+2].x() - grid[i+3].x(), cellSideSize);
        QVERIFY(grid[i+2].y() == grid[i+3].y());

        QCOMPARE(grid[i+3].y() - grid[i].y(), cellSideSize);
        QVERIFY(grid[i+3].x() == grid[i].x());
    }
}

void TestBasic::generateZeroSizeQuadGridTestCase()
{
    Point3D <float> topLeft(0.0f,0.0f,0.0f);
    int width = 0;
    int height = 0;
    float cellSideSize = 0.2f;

    auto grid = GridGenerator <float>::generateQuadGrid(topLeft, width, height, cellSideSize);

    QCOMPARE(grid.size(), static_cast <size_t>(0));
}

void TestBasic::generateZeroCellSizeQuadGridTestCase()
{
    Point3D <float> topLeft(0.0f,0.0f,0.0f);
    int width = 10;
    int height = 20;
    float cellSideSize = 0.0f;

    auto grid = GridGenerator <float>::generateQuadGrid(topLeft, width, height, cellSideSize);

    QVERIFY(grid.size() > 4);
    QVERIFY(grid.size() % 4 == 0);
    QCOMPARE(grid.size(), static_cast <size_t>(width * height * 4));

    for (size_t i = 0; i < grid.size()-1; i++){

        QVERIFY(grid[i].x() == grid[i+1].x());
        QVERIFY(grid[i].y() == grid[i+1].y());
    }
}

void TestBasic::cleanupTestCase()
{

}

QTEST_MAIN(TestBasic)

//#include "tst_basic.moc"
