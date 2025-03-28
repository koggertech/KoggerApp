#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <console_list_model.h>
#include <QLoggingCategory>
#include <QQuickTextDocument>

class Console : public QObject
{
    Q_OBJECT
public:
    Console();
    ConsoleListModel* listModel() const;

    void put(QtMsgType type, const QString &msg);

public slots:


private:
    ConsoleListModel *m_list;
};

#endif // CONSOLE_H
