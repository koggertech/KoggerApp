#ifndef PARSER_H
#define PARSER_H

#include <QObject>
#include <ProtoBinnary.h>

class Parser : public QObject
{
    Q_OBJECT
public:
    explicit Parser();

public slots:
    void putData(const QByteArray &data);

signals:
    void complete();
    void receiveDataChart();
    void receiveSettingsChart();

private:
    bool putBinnary(char b);

    ProtBin* m_binnary;

private slots:
    void doBynnaryParse();
};

#endif // PARSER_H
