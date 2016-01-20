#ifndef MAINWIDGET_H
#define MAINWIDGET_H
#pragma once

#include <QWidget>

class QImage;
class QNetworkReply;
class QSystemTrayIcon;

namespace Ui { class MainWidget; }


class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = Q_NULLPTR);
    virtual ~MainWidget();

public slots:
    void startDownload();

private slots:
    void browseOutputDir();
    void saveAndClose();
    void handleReply(QNetworkReply *reply);

private:
    void handleArchiveReply(QNetworkReply *reply);
    void handleImageReply(QNetworkReply *reply);
    void paintCopyright(QImage &image) const;

private:
    Ui::MainWidget *ui;
    QSystemTrayIcon *tray_;
    QString copyright_;
};


#endif // MAINWIDGET_H
