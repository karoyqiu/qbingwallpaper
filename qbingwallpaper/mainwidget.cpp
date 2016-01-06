#include "stdafx.h"
#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QJsonDocument>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSystemTrayIcon>


static void paintCopyright(QImage *image, const QString &copyright)
{
    QSettings settings;
    settings.setValue("fontFamily", "Microsoft YaHei");

    QFont font(settings.value(QS("fontFamily"), QS("Sans")).toString(),
               settings.value(QS("fontSize"), -1).toInt());

    QScreen *screen = QGuiApplication::primaryScreen();
    qreal ratio = image->height();
    ratio /= screen->size().height();
    qDebug() << "Ratio =" << ratio;

    font.setPointSizeF(font.pointSizeF() * ratio);

    QFontMetricsF metrics(font);
    QRectF rect = metrics.boundingRect(copyright);
    rect.setWidth(rect.width() * 1.5);

    int x = settings.value(QS("x"), 32).toInt();
    int y = settings.value(QS("y"), 32).toInt();
    rect.moveRight(image->width() - (x - 1) * ratio);
    rect.moveTop((y + 1) * ratio);

    QPainter painter(image);
    painter.setFont(font);
    painter.setPen(Qt::black);
    painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter, copyright);
    painter.setPen(QColor("whitesmoke"));
    painter.drawText(rect.translated(-ratio, -ratio), Qt::AlignRight | Qt::AlignVCenter, copyright);
}


static QString outputDirectory()
{
#ifdef Q_OS_WIN
    const QSettings reg(QS(R"(HKEY_CURRENT_USER\SOFTWARE\Microsoft\BingWallPaper\Config)"),
                        QSettings::NativeFormat);
    QString defaultLocation = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    return reg.value(QS("WallPaperFolder"), defaultLocation).toString();
#endif
}


static bool setWallpaper(const QString &fileName)
{
#ifdef Q_OS_WIN
    WCHAR wszFileName[MAX_PATH] = { 0 };
    fileName.toWCharArray(wszFileName);
    return SystemParametersInfoW(SPI_SETDESKWALLPAPER, static_cast<UINT>(wcslen(wszFileName)), wszFileName,
                                 SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) != FALSE;
#endif
}


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


void MainWidget::startDownload()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &MainWidget::handleReply);

    qDebug() << "Getting image URL...";
    tray_->setToolTip(tr("Getting image URL..."));

    QNetworkRequest request(QUrl(QS("https://www.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1")));
    manager->get(request);
}


void MainWidget::handleReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        if (copyright_.isEmpty())
        {
            handleArchiveReply(reply);
        }
        else
        {
            handleImageReply(reply);
        }
    }

    reply->deleteLater();
}


void MainWidget::handleArchiveReply(QNetworkReply * reply)
{
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    Q_ASSERT(doc.isObject());

    QJsonObject obj = doc.object();
    Q_ASSERT(obj.contains(QS("images")));

    QJsonValue value = obj.value(QS("images"));
    Q_ASSERT(value.isArray());

    obj = value.toArray().first().toObject();
    copyright_ = obj.value("copyright").toString();
    QString url = QS("https://www.bing.com") % obj.value(QS("url")).toString();
    qDebug() << copyright_;
    qDebug() << url;

    qDebug() << "Downloading image...";
    tray_->setToolTip(tr("Downloading image..."));

    QNetworkAccessManager *manager = reply->manager();
    manager->get(QNetworkRequest(QUrl(url)));
}


void MainWidget::handleImageReply(QNetworkReply *reply)
{
    QImage image = QImage::fromData(reply->readAll());
    paintCopyright(&image, copyright_);

    QFileInfo info(reply->url().fileName());
    QDir dir(outputDirectory());
    QString fileName = dir.absoluteFilePath(info.completeBaseName() % QS(".png"));

    qDebug() << "Setting wallpaper...";

    if (image.save(fileName) && setWallpaper(QDir::toNativeSeparators(fileName)))
    {
        close();
    }
    else
    {
        qCritical() << "Failed to save image.";
    }
}
