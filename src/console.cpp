#include "console.h"
#include "QTime"

Console::Console()
    :m_list(new ConsoleListModel())
    ,m_app(new ConsoleListModel())
    ,m_proto(new ConsoleListModel())
{
    m_list->init();
    m_app->init();
    m_proto->init();
}

ConsoleListModel *Console::listModel() const {
    return m_list;
}

ConsoleListModel *Console::appModel() const {
    return m_app;
}

ConsoleListModel *Console::protoModel() const {
    return m_proto;
}

void Console::put(QtMsgType type, const QString &msg, ConsoleSource source) {
    const QString time = QTime::currentTime().toString(QStringLiteral("hh:mm:ss:zzz"));

    const bool multiline = msg.contains(QLatin1Char('\n')) || msg.contains(QLatin1Char('\r'));
    const QString line = multiline ? msg.simplified() : msg;

    Q_EMIT m_list->appendEvent(time, type, line);

    ConsoleListModel* sourceList = source == ConsoleSource::Proto ? m_proto : m_app;
    Q_EMIT sourceList->appendEvent(time, type, line);
}

void Console::setMaxRows(int rows) {
    m_list->setMaxRows(rows);
    m_app->setMaxRows(rows);
    m_proto->setMaxRows(rows);
}
