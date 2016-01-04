#ifndef MAINWIDGET_H
#define MAINWIDGET_H
#pragma once

#include <QWidget>

class QSystemTrayIcon;

namespace Ui { class MainWidget; }


class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = Q_NULLPTR);
    virtual ~MainWidget();

private:
    Ui::MainWidget *ui;
    QSystemTrayIcon *tray_;
};


#endif // MAINWIDGET_H
