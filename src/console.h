#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <console_list_model.h>
#include <QLoggingCategory>
#include <QQuickTextDocument>

enum class ConsoleSource : uint8_t {
    App,
    Proto,
};

class Console : public QObject
{
    Q_OBJECT
public:
    Console();
    ConsoleListModel* listModel() const;
    ConsoleListModel* appModel() const;
    ConsoleListModel* protoModel() const;

    void put(QtMsgType type, const QString &msg, ConsoleSource source = ConsoleSource::App);
    void setMaxRows(int rows);

public slots:


private:
    ConsoleListModel *m_list;
    ConsoleListModel *m_app;
    ConsoleListModel *m_proto;
};

#endif // CONSOLE_H
