#pragma once

#include <QObject>
#include <QSettings>
#include <QTranslator>
#include <QVector>
#include <QString>
#include <QLocale>
#include <QGuiApplication>


class LanguageController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
public:
    explicit LanguageController(QObject* parent = nullptr) : QObject(parent) {
        QSettings s;
        int idx = s.value("appLanguage", -1).toInt();
        if (idx < 0 || idx >= langs_.size()) {
            QString sys = QLocale::system().name().split('_').first();
            idx = langs_.indexOf(sys);
            if (idx < 0) idx = 0;
        }
        index_ = idx;
    }

    int currentIndex() const { return index_; }

    // Call once after loadLanguage() so we can remove the startup translator on first switch
    void setStartupTranslator(QTranslator* t) { startupTranslator_ = t; }

    Q_INVOKABLE void apply(int i) {
        if (i < 0 || i >= langs_.size() || i == index_) return;
        index_ = i;
        QSettings s;
        s.setValue("appLanguage", i);
        if (startupTranslator_) {
            qGuiApp->removeTranslator(startupTranslator_);
            startupTranslator_ = nullptr;
        }
        qGuiApp->removeTranslator(&translator_);
        if (translator_.load(":/translations/translation_" + langs_[i] + ".qm"))
            qGuiApp->installTranslator(&translator_);
        emit currentIndexChanged();
    }

signals:
    void currentIndexChanged();

private:
    QTranslator  translator_;
    QTranslator* startupTranslator_{nullptr};
    QVector<QString> langs_{"en", "ru", "pl"};
    int index_{0};
};
