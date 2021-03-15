#include "console.h"
#include "QTime"

Console::Console()
    :m_list(new ConsoleListModel())
{
    m_list->init();
}

ConsoleListModel *Console::listModel() const {
    return m_list;
}

void Console::put(QtMsgType type, const QString &msg) {
    const QString time = QTime::currentTime().toString(QStringLiteral("hh:mm:ss:zzz"));
    m_list->appendEvent(time, type, msg);
}
