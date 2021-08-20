#ifndef THEME_H
#define THEME_H

#include <QObject>
#include <QFont>
#include <QColor>

class Themes : public QObject
{
    Q_OBJECT
public:

    Themes() {
        setTheme();
    }

    Q_PROPERTY(QColor textColor READ textColor NOTIFY changed)
    Q_PROPERTY(QFont textFont READ textFont NOTIFY changed)
    Q_PROPERTY(QFont textFontS READ textFontS NOTIFY changed)

    Q_PROPERTY(QColor menuBackColor READ menuBackColor NOTIFY changed)
    Q_PROPERTY(QColor controlBackColor READ controlBackColor NOTIFY changed)
    Q_PROPERTY(QColor controlBorderColor READ controlBorderColor NOTIFY changed)
    Q_PROPERTY(QColor controlSolidBackColor READ controlSolidBackColor NOTIFY changed)
    Q_PROPERTY(QColor controlSolidBorderColor READ controlSolidBorderColor NOTIFY changed)

    Q_PROPERTY(QColor themeID READ menuBackColor NOTIFY changed)

    QColor textColor() { return *_textColor; }
    QFont textFont() { return *_textFont; }
    QFont textFontS() { return *_textFontS; }

    QColor menuBackColor() { return *_menuBackColor; }
    QColor controlBackColor() { return *_controlBackColor; }
    QColor controlBorderColor() { return *_controlBorderColor; }
    QColor controlSolidBackColor() { return *_controlSolidBackColor; }
    QColor controlSolidBorderColor() { return *_controlSolidBorderColor; }

    void setTheme(int theme_id = 0) {
        _id = theme_id;
        _textFont = new QFont("PT Sans Caption", 13);
        _textFontS = new QFont("PT Sans Caption", 11);

        if(theme_id == 0) {
            _textColor = new QColor(250, 250, 250);
            _menuBackColor = new QColor(45, 45, 45, 240);
            _controlBackColor = new QColor(60, 60, 60);
            _controlBorderColor = new QColor(100, 100, 100);
            _controlSolidBackColor = new QColor(80, 80, 80);
            _controlSolidBorderColor = new QColor(150, 150, 150);
        } else if(theme_id == 1) {
            _textColor = new QColor(25, 25, 25);
            _menuBackColor = new QColor(240, 240, 240, 240);
            _controlBackColor = new QColor(250, 250, 250);
            _controlBorderColor = new QColor(100, 100, 100);
            _controlSolidBackColor = new QColor(255, 255, 255);
            _controlSolidBorderColor = new QColor(150, 150, 150);
        }
    }

signals:
    void changed();
protected:
    int _id = 0;

    QColor* _textColor;
    QFont* _textFont;
    QFont* _textFontS;

    QColor* _menuBackColor;
    QColor* _controlBackColor;
    QColor* _controlBorderColor;
    QColor* _controlSolidBackColor;
    QColor* _controlSolidBorderColor;
};

#endif // THEME_H
