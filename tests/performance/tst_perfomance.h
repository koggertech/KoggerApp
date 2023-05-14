#ifndef TST_PERFOMANCE_H
#define TST_PERFOMANCE_H

#include <QtTest>

class TestPerformance : public QObject
{
    Q_OBJECT

public:
    TestPerformance();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
};


#endif // TST_PERFOMANCE_H
