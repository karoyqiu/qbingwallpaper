#include "stdafx.h"

#include <QApplication>

#include "mainwidget.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName(QS("qbingwallpaper"));
    QApplication::setApplicationDisplayName(QS("QBingWallpaper"));
    QApplication::setOrganizationName(QS("Roy QIU"));
    QApplication::setOrganizationDomain(QS("karoyqiu.com"));

    MainWidget w;

    return a.exec();
}
