#include "stdafx.h"

#include <QApplication>
#include <QCommandLineParser>

#include "mainwidget.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName(QS("qbingwallpaper"));
    QApplication::setApplicationDisplayName(QS("QBingWallpaper"));
    QApplication::setApplicationVersion(QS("2.0"));
    QApplication::setOrganizationName(QS("Roy QIU"));
    QApplication::setOrganizationDomain(QS("karoyqiu.com"));


    QLocale locale;
    QTranslator translatorQt;

    if (translatorQt.load(locale, QS("qt"), QS("_"), QApplication::applicationDirPath()))
    {
        QApplication::installTranslator(&translatorQt);
    }
    else
    {
        qWarning() << "Failed to load translation file for Qt.";
    }

    QTranslator translatorApp;

    if (translatorApp.load(locale, QApplication::applicationName(), QS("_"), QApplication::applicationDirPath()))
    {
        QApplication::installTranslator(&translatorApp);
    }
    else
    {
        qWarning() << "Failed to load translation file for" << qPrintable(QApplication::applicationName());
    }


    QCommandLineParser parser;
    parser.setApplicationDescription(QApplication::translate("main", "Download and set today's Bing wallpaper."));
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions({
        { { "a", "auto", "autorun" }, QApplication::translate("main", "Automatically download and set the wallpaper.") }
    });

    parser.process(a);

    MainWidget w;

    if (parser.isSet("a"))
    {
        w.startDownload();
    }
    else
    {
        w.show();
    }

    return QApplication::exec();
}
