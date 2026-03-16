#ifndef THEME_H
#define THEME_H

#include <QObject>
#include <QFont>
#include <QColor>
#include <QScreen>
#include <QDebug>
#include <QtGlobal>


class Themes : public QObject
{
    Q_OBJECT

public:

    Themes() :
        instrumentsGrade_(-1),
        resolutionCoeff_(1.0)
    {
        setTheme();
        _isConsoleVisible = false;
    }

    Q_PROPERTY(qreal resCoeff READ getResolutionCoeff NOTIFY changed)

    Q_PROPERTY(QColor disabledTextColor READ disabledTextColor NOTIFY changed)
    Q_PROPERTY(QColor disabledBackColor READ disabledBackColor NOTIFY changed)
    Q_PROPERTY(QColor hoveredBackColor READ hoveredBackColor NOTIFY changed)

    Q_PROPERTY(QColor textColor READ textColor NOTIFY changed)
    Q_PROPERTY(QColor textSolidColor READ textSolidColor NOTIFY changed)
    Q_PROPERTY(QColor textErrorColor READ textErrorColor NOTIFY changed)
    Q_PROPERTY(QFont textFont READ textFont NOTIFY changed)
    Q_PROPERTY(QFont textFontS READ textFontS NOTIFY changed)

    Q_PROPERTY(QColor menuBackColor READ menuBackColor NOTIFY changed)
    Q_PROPERTY(QColor frameBackColor READ frameBackColor NOTIFY changed)

    Q_PROPERTY(QColor controlBackColor READ controlBackColor NOTIFY changed)
    Q_PROPERTY(QColor controlBorderColor READ controlBorderColor NOTIFY changed)
    Q_PROPERTY(QColor controlSolidBackColor READ controlSolidBackColor NOTIFY changed)
    Q_PROPERTY(QColor controlSolidBorderColor READ controlSolidBorderColor NOTIFY changed)
    Q_PROPERTY(QColor activeControlBackColor READ activeControlBackColor NOTIFY changed)
    Q_PROPERTY(QColor sliderHandleColor READ sliderHandleColor NOTIFY changed)
    Q_PROPERTY(QColor sliderHandlePressedColor READ sliderHandlePressedColor NOTIFY changed)
    Q_PROPERTY(QColor placeholderTextColor READ placeholderTextColor NOTIFY changed)
    Q_PROPERTY(QColor tooltipBackColor READ tooltipBackColor NOTIFY changed)
    Q_PROPERTY(QColor tooltipBorderColor READ tooltipBorderColor NOTIFY changed)
    Q_PROPERTY(QColor tooltipTextColor READ tooltipTextColor NOTIFY changed)
    Q_PROPERTY(int controlHeight READ controlHeight NOTIFY changed)
    Q_PROPERTY(int menuWidth READ menuWidth NOTIFY changed)

    Q_PROPERTY(int themeID READ themeId WRITE setTheme NOTIFY changed)

    Q_PROPERTY(bool consoleVisible READ consoleVisible WRITE setConsoleVisible NOTIFY interfaceChanged)
    Q_PROPERTY(int instrumentsGrade READ getInstrumentsGrade WRITE setInstrumentsGrade NOTIFY instrumentsGradeChanged)

    Q_INVOKABLE void updateResCoeff() {
        qreal currCoeff = checkResolutionCoeff();

        if (!qFuzzyCompare(1.0 + currCoeff, 1.0 + resolutionCoeff_)) {
            resolutionCoeff_ = currCoeff;
            emit changed();
        }
    };

    qreal getResolutionCoeff() const { return resolutionCoeff_; };
    QColor textColor() { return *_textColor; }
    QColor textErrorColor() { return *_textErrorColor; }
    QColor disabledTextColor() { return *_disabledTextColor; }
    QColor disabledBackColor() { return *_disabledBackColor; }
    QColor hoveredBackColor() { return *_hoveredBackColor; }
    QColor textSolidColor() { return *_textSolidColor; }
    QFont textFont() { return *_textFont; }
    QFont textFontS() { return *_textFontS; }

    QColor menuBackColor() { return *_menuBackColor; }
    QColor frameBackColor() { return *_frameBackColor; }

    QColor controlBackColor() { return *_controlBackColor; }
    QColor controlBorderColor() { return *_controlBorderColor; }
    QColor controlSolidBackColor() { return *_controlSolidBackColor; }
    QColor controlSolidBorderColor() { return *_controlSolidBorderColor; }
    QColor activeControlBackColor() { return *_activeControlBackColor; }
    QColor sliderHandleColor() { return *_sliderHandleColor; }
    QColor sliderHandlePressedColor() { return *_sliderHandlePressedColor; }
    QColor placeholderTextColor() { return *_placeholderTextColor; }
    QColor tooltipBackColor() { return *_tooltipBackColor; }
    QColor tooltipBorderColor() { return *_tooltipBorderColor; }
    QColor tooltipTextColor() { return *_tooltipTextColor; }
    int controlHeight() { return _controlHeight; }
    int menuWidth() { return _menuWidth; }

    Q_INVOKABLE QColor invertedColor(const QColor& color) const {
        return QColor::fromRgbF(1.0 - color.redF(), 1.0 - color.greenF(), 1.0 - color.blueF(), color.alphaF());
    }

    Q_INVOKABLE qreal contrastRatio(const QColor& first, const QColor& second) const {
        const qreal firstL = first.redF() * 0.299 + first.greenF() * 0.587 + first.blueF() * 0.114;
        const qreal secondL = second.redF() * 0.299 + second.greenF() * 0.587 + second.blueF() * 0.114;
        const qreal l1 = qMax(firstL, secondL);
        const qreal l2 = qMin(firstL, secondL);
        return (l1 + 0.05) / (l2 + 0.05);
    }

    Q_INVOKABLE QColor contrastTextColor(const QColor& background) const {
        const qreal l = background.redF() * 0.299 + background.greenF() * 0.587 + background.blueF() * 0.114;
        return l > 0.62 ? QColor(26, 26, 26, 255) : QColor(245, 245, 245, 255);
    }

    void setTheme(int theme_id = 0) {
        _id = theme_id;

#if defined(Q_OS_ANDROID)
        _textFont = new QFont("Times", 14);
        _textFont->setPixelSize(30);
        _textFontS = new QFont("Times", 12);
        _textFontS->setPixelSize(24);
#else
        _textFont = new QFont("PT Sans Caption", 14);
        _textFont->setPixelSize(22);
        _textFontS = new QFont("PT Sans Caption", 12);
        _textFont->setPixelSize(18);
#endif
        _textErrorColor = new QColor(250, 0, 0);

        _frameBackColor = new QColor(45, 45, 45, 50);

        if(theme_id == 0) {
            _textColor = new QColor(250, 250, 250);
            _textSolidColor = new QColor(250, 250, 250);
            _menuBackColor = new QColor(45, 45, 45, 240);
            _controlBackColor = new QColor(60, 60, 60);
            _controlBorderColor = new QColor(100, 100, 100);
            _controlSolidBackColor = new QColor(100, 100, 100);
            _controlSolidBorderColor = new QColor(150, 150, 150);
            _activeControlBackColor = new QColor(230, 230, 230);
            _sliderHandleColor = new QColor(209, 209, 209);
            _sliderHandlePressedColor = new QColor(230, 230, 230);
            _placeholderTextColor = new QColor(235, 235, 235, 230);
            _tooltipBackColor = new QColor(41, 41, 41, 235);
            _tooltipBorderColor = new QColor(255, 255, 255, 46);
            _tooltipTextColor = new QColor(245, 245, 245);

            _disabledTextColor = new QColor(150, 150, 150);
            _disabledBackColor = new QColor(50, 50, 50);
            _hoveredBackColor = new QColor(70,70,70);

        } else if(theme_id == 1) {
            _textColor = new QColor(255, 255, 255);
            _textSolidColor = new QColor(255, 255, 255);
            _menuBackColor = new QColor(0, 0, 0, 255);
            _controlBackColor = new QColor(55, 55, 55);
            _controlBorderColor = new QColor(155, 155, 155);
            _controlSolidBackColor = new QColor(85, 85, 85);
            _controlSolidBorderColor = new QColor(180, 180, 180);
            _activeControlBackColor = new QColor(255, 255, 255);
            _sliderHandleColor = new QColor(230, 230, 230);
            _sliderHandlePressedColor = new QColor(245, 245, 245);
            _placeholderTextColor = new QColor(250, 250, 250, 245);
            _tooltipBackColor = new QColor(41, 41, 41, 235);
            _tooltipBorderColor = new QColor(255, 255, 255, 46);
            _tooltipTextColor = new QColor(245, 245, 245);

            _disabledTextColor = new QColor(150, 150, 150);
            _disabledBackColor = new QColor(50, 50, 50);
            _hoveredBackColor = new QColor(80, 80, 80);

        } else if(theme_id == 2) {
            _textColor = new QColor(25, 25, 25);
            _textSolidColor = new QColor(25, 25, 25);
            _menuBackColor = new QColor(206, 206, 206, 245);
            _controlBackColor = new QColor(222, 222, 222);
            _controlBorderColor = new QColor(118, 118, 118);
            _controlSolidBackColor = new QColor(232, 232, 232);
            _controlSolidBorderColor = new QColor(172, 172, 172);
            _activeControlBackColor = new QColor(51, 51, 51);
            _sliderHandleColor = new QColor(77, 77, 77);
            _sliderHandlePressedColor = new QColor(61, 61, 61);
            _placeholderTextColor = new QColor(26, 26, 26, 143);
            _tooltipBackColor = new QColor(255, 255, 255, 242);
            _tooltipBorderColor = new QColor(0, 0, 0, 46);
            _tooltipTextColor = new QColor(20, 20, 20);

            _disabledTextColor = new QColor(150, 150, 150);
            _disabledBackColor = new QColor(50, 50, 50);
            _hoveredBackColor = new QColor(216, 216, 216);

        } else if(theme_id == 3) {
            _textColor = new QColor(0, 0, 0);
            _textSolidColor = new QColor(0, 0, 0);
            _menuBackColor = new QColor(236, 236, 236, 250);
            _controlBackColor = new QColor(252, 252, 252);
            _controlBorderColor = new QColor(120, 120, 120);
            _controlSolidBackColor = new QColor(252, 252, 252);
            _controlSolidBorderColor = new QColor(155, 155, 155);
            _activeControlBackColor = new QColor(0, 0, 0);
            _sliderHandleColor = new QColor(51, 51, 51);
            _sliderHandlePressedColor = new QColor(36, 36, 36);
            _placeholderTextColor = new QColor(13, 13, 13, 163);
            _tooltipBackColor = new QColor(255, 255, 255, 242);
            _tooltipBorderColor = new QColor(0, 0, 0, 46);
            _tooltipTextColor = new QColor(20, 20, 20);

            _disabledTextColor = new QColor(150, 150, 150);
            _disabledBackColor = new QColor(50, 50, 50);
            _hoveredBackColor = new QColor(244, 244, 244);
        } else if(theme_id == 4) {
            _textColor = new QColor(220, 223, 231);
            _textSolidColor = new QColor(220, 223, 231);
            _menuBackColor = new QColor(40, 44, 52, 245);
            _controlBackColor = new QColor(54, 58, 68);
            _controlBorderColor = new QColor(97, 104, 122);
            _controlSolidBackColor = new QColor(69, 75, 89);
            _controlSolidBorderColor = new QColor(129, 138, 160);
            _activeControlBackColor = new QColor(236, 239, 246);
            _sliderHandleColor = new QColor(184, 191, 206);
            _sliderHandlePressedColor = new QColor(208, 214, 227);
            _placeholderTextColor = new QColor(214, 219, 230, 224);
            _tooltipBackColor = new QColor(41, 45, 54, 238);
            _tooltipBorderColor = new QColor(255, 255, 255, 54);
            _tooltipTextColor = new QColor(245, 247, 250);

            _disabledTextColor = new QColor(144, 150, 164);
            _disabledBackColor = new QColor(44, 48, 58);
            _hoveredBackColor = new QColor(64, 70, 84);
        } else if(theme_id == 5) {
            _textColor = new QColor(248, 248, 242);
            _textSolidColor = new QColor(248, 248, 242);
            _menuBackColor = new QColor(39, 40, 34, 245);
            _controlBackColor = new QColor(56, 56, 48);
            _controlBorderColor = new QColor(106, 106, 92);
            _controlSolidBackColor = new QColor(72, 72, 62);
            _controlSolidBorderColor = new QColor(142, 142, 124);
            _activeControlBackColor = new QColor(244, 244, 236);
            _sliderHandleColor = new QColor(198, 202, 191);
            _sliderHandlePressedColor = new QColor(220, 223, 214);
            _placeholderTextColor = new QColor(236, 236, 226, 220);
            _tooltipBackColor = new QColor(45, 45, 38, 238);
            _tooltipBorderColor = new QColor(255, 255, 255, 54);
            _tooltipTextColor = new QColor(248, 248, 242);

            _disabledTextColor = new QColor(145, 145, 136);
            _disabledBackColor = new QColor(45, 45, 39);
            _hoveredBackColor = new QColor(66, 66, 57);
        } else if(theme_id == 6) {
            _textColor = new QColor(226, 205, 173);
            _textSolidColor = new QColor(226, 205, 173);
            _menuBackColor = new QColor(33, 26, 16, 246);
            _controlBackColor = new QColor(48, 38, 24);
            _controlBorderColor = new QColor(107, 89, 66);
            _controlSolidBackColor = new QColor(63, 49, 31);
            _controlSolidBorderColor = new QColor(138, 118, 90);
            _activeControlBackColor = new QColor(245, 233, 212);
            _sliderHandleColor = new QColor(202, 175, 136);
            _sliderHandlePressedColor = new QColor(221, 197, 162);
            _placeholderTextColor = new QColor(223, 204, 173, 224);
            _tooltipBackColor = new QColor(40, 31, 20, 238);
            _tooltipBorderColor = new QColor(255, 255, 255, 56);
            _tooltipTextColor = new QColor(242, 224, 196);

            _disabledTextColor = new QColor(141, 126, 104);
            _disabledBackColor = new QColor(38, 30, 20);
            _hoveredBackColor = new QColor(74, 59, 39);
        } else if(theme_id == 7) {
            _textColor = new QColor(147, 161, 161);
            _textSolidColor = new QColor(147, 161, 161);
            _menuBackColor = new QColor(0, 43, 54, 246);
            _controlBackColor = new QColor(7, 54, 66);
            _controlBorderColor = new QColor(88, 110, 117);
            _controlSolidBackColor = new QColor(18, 67, 80);
            _controlSolidBorderColor = new QColor(101, 123, 131);
            _activeControlBackColor = new QColor(238, 232, 213);
            _sliderHandleColor = new QColor(147, 161, 161);
            _sliderHandlePressedColor = new QColor(168, 180, 181);
            _placeholderTextColor = new QColor(140, 154, 154, 220);
            _tooltipBackColor = new QColor(7, 54, 66, 240);
            _tooltipBorderColor = new QColor(238, 232, 213, 56);
            _tooltipTextColor = new QColor(238, 232, 213);

            _disabledTextColor = new QColor(101, 123, 131);
            _disabledBackColor = new QColor(5, 38, 48);
            _hoveredBackColor = new QColor(14, 63, 76);
        }
#if defined(Q_OS_ANDROID)
        _controlHeight = 48;
#elif defined(LINUX_ES)
        _controlHeight = 38;
#else
        _controlHeight = 26;
#endif
        emit changed();
    }

    int themeId() { return _id; }

    void setConsoleVisible(bool vis) {
        _isConsoleVisible = vis;
        emit interfaceChanged();
    }

    bool consoleVisible()
    {
        return _isConsoleVisible;
    }

    int getInstrumentsGrade() const
    {
        return instrumentsGrade_;
    }

    void setInstrumentsGrade(int instrumentsGrade)
    {
        instrumentsGrade_ = instrumentsGrade;
        emit instrumentsGradeChanged();
    }

signals:
    void changed();
    void interfaceChanged();
    void instrumentsGradeChanged();

protected:
    int _id = 0;

    QColor* _textColor;
    QColor* _textSolidColor;
    QColor* _textErrorColor;
    QColor* _disabledTextColor;
    QColor* _disabledBackColor;
    QColor* _hoveredBackColor;
    QFont* _textFont;
    QFont* _textFontS;

    QColor* _menuBackColor;
    QColor* _frameBackColor;
    QColor* _controlBackColor;
    QColor* _controlBorderColor;
    QColor* _controlSolidBackColor;
    QColor* _controlSolidBorderColor;
    QColor* _activeControlBackColor;
    QColor* _sliderHandleColor;
    QColor* _sliderHandlePressedColor;
    QColor* _placeholderTextColor;
    QColor* _tooltipBackColor;
    QColor* _tooltipBorderColor;
    QColor* _tooltipTextColor;
    int32_t _controlHeight;
    int32_t _menuWidth;

    bool _isConsoleVisible;
    int instrumentsGrade_;
private:
    qreal checkResolutionCoeff() const;
    qreal resolutionCoeff_;
};

inline qreal Themes::checkResolutionCoeff() const
{
    qreal retVal = 1.0;

#if defined(Q_OS_ANDROID) || defined(LINUX_ES)
    retVal = 2.0;
#endif

    // QScreen *screen = QApplication::primaryScreen();
    // if (screen) {
    //     auto physDotsPerInch = screen->physicalDotsPerInch();
    //     Q_UNUSED(physDotsPerInch)


    //     // retVal = 2.0; // test
    //     // qDebug() << "Logical DPI:" << screen->logicalDotsPerInch();
    //     // qDebug() << "Physical DPI:" << screen->physicalDotsPerInch();
    //     // qDebug() << "Device Pixel Ratio:" << screen->devicePixelRatio();
    // }

    return retVal;
}

#endif // THEME_H
