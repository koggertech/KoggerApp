#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <consolelistmodel.h>
#include <QLoggingCategory>

class Console : public QObject
{
    Q_OBJECT
public:
    Console();
    ConsoleListModel* listModel() const;

    void put(QtMsgType type, const QString &msg);

private:
    ConsoleListModel *m_list;
};

#endif // CONSOLE_H
