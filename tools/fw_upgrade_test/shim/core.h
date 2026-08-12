#pragma once

// Stand-in for src/core.h. dev_driver.cpp and id_binnary.cpp reach the application
// singleton for one thing only -- consoleInfo() -- and pulling the real Core in would
// drag the whole app (scene, dataset, QML) into a protocol test. Search order in run.ps1
// puts this directory ahead of src/, so <core.h> resolves here.
//
// The captured lines are part of the contract under test: the upgrade state machine
// reports every one of its transitions through consoleInfo and nowhere else, so a
// scenario that asserts "the host gave up" asserts on this list.

#include <QString>
#include <QStringList>

class Core
{
public:
    void consoleInfo(const QString& text)    { lines_.append(text); }
    void consoleWarning(const QString& text) { lines_.append(text); }

    QStringList takeLines() { QStringList out = lines_; lines_.clear(); return out; }
    const QStringList& lines() const { return lines_; }
    int countContaining(const QString& needle) const {
        int n = 0;
        for (const QString& l : lines_) { if (l.contains(needle)) ++n; }
        return n;
    }
    void clear() { lines_.clear(); }

private:
    QStringList lines_;
};

extern Core core;
