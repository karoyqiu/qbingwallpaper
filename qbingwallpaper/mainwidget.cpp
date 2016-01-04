#include "stdafx.h"
#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QMenu>
#include <QSystemTrayIcon>


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , tray_(Q_NULLPTR)
{
    ui = new Ui::MainWidget();
    ui->setupUi(this);

    tray_ = q_check_ptr(new QSystemTrayIcon(windowIcon(), this));
    tray_->setToolTip(windowTitle());
    tray_->show();

    QMenu *menu = new QMenu(this);
    menu->addAction(ui->actionExit);
    tray_->setContextMenu(menu);
}


MainWidget::~MainWidget()
{
    delete ui;
}
