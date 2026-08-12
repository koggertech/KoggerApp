#pragma once

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
