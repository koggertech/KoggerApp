#include "parser.h"

Parser::Parser() :
    QObject(),
    m_binnary(new ProtBin())
{
    connect(m_binnary, &ProtBin::complete, this, &Parser::doBynnaryParse);
}

void Parser::putData(const QByteArray &data) {
    for(int i = 0; i < data.size(); i++) {
        putBinnary(data.at(i));
    }
}

bool Parser::putBinnary(char b) {
    m_binnary->putByte((uint8_t)(b));
    return true;
}

void Parser::doBynnaryParse() {

}

