#include "stdafx.h"
#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QJsonDocument>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStandardPaths>
#include <QSystemTrayIcon>


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
    ui = q_check_ptr(new Ui::MainWidget());
    ui->setupUi(this);

    const QSettings settings;
    QString defaultLocation = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

#ifdef Q_OS_WIN
    {
        const QSettings reg(QS(R"(HKEY_CURRENT_USER\SOFTWARE\Microsoft\BingWallPaper\Config)"),
                            QSettings::NativeFormat);
        defaultLocation = reg.value(QS("WallPaperFolder"), defaultLocation).toString();
    }
#endif

    defaultLocation = settings.value(QS("outputDir"), defaultLocation).toString();
    ui->editOutputDir->setText(defaultLocation);
    ui->groupCopyright->setChecked(settings.value(QS("printCopyright"), true).toBool());

    QFont font(settings.value(QS("fontFamily"), tr("Sans")).toString(),
               settings.value(QS("fontSize"), 0).toInt());
    ui->fontCombo->setCurrentFont(font);
    ui->spinFontSize->setValue(font.pointSize());

    connect(ui->buttonBrowseOutputDir, &QToolButton::clicked, this, &MainWidget::browseOutputDir);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &MainWidget::saveAndClose);
}


MainWidget::~MainWidget()
{
    delete ui;
}


void MainWidget::startDownload()
{
    tray_ = q_check_ptr(new QSystemTrayIcon(windowIcon(), this));
    tray_->setToolTip(windowTitle());
    tray_->show();

    QMenu *menu = q_check_ptr(new QMenu(this));
    menu->addAction(ui->actionExit);
    tray_->setContextMenu(menu);


    QNetworkAccessManager *manager = q_check_ptr(new QNetworkAccessManager(this));
    connect(manager, &QNetworkAccessManager::finished, this, &MainWidget::handleReply);

    qDebug() << "Getting image URL...";
    tray_->setToolTip(tr("Getting image URL..."));

    QNetworkRequest request(QUrl(QS("https://www.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1")));
    manager->get(request);
}


void MainWidget::browseOutputDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, QString(), ui->editOutputDir->text());

    if (!dir.isEmpty())
    {
        ui->editOutputDir->setText(QDir::toNativeSeparators(dir));
    }
}


void MainWidget::saveAndClose()
{
    QSettings settings;
    settings.setValue(QS("outputDir"), ui->editOutputDir->text());
    settings.setValue(QS("printCopyright"), ui->groupCopyright->isChecked());
    settings.setValue(QS("fontFamily"), ui->fontCombo->currentFont().family());
    settings.setValue(QS("fontSize"), ui->spinFontSize->value());

    close();
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
    else
    {
        QMessageBox::critical(this, tr("Failed to download"), reply->errorString());
        close();
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

    qDebug() << "Downloading" << copyright_ << "from" << url;
    tray_->setToolTip(tr("Downloading image..."));

    QNetworkAccessManager *manager = reply->manager();
    manager->get(QNetworkRequest(QUrl(url)));
}


void MainWidget::handleImageReply(QNetworkReply *reply)
{
    QImage image = QImage::fromData(reply->readAll());

    if (ui->groupCopyright->isChecked())
    {
        paintCopyright(image);
    }

    QFileInfo info(reply->url().fileName());
    QDir dir(ui->editOutputDir->text());
    QString fileName = dir.absoluteFilePath(info.completeBaseName() % QS(".png"));

    qDebug() << "Setting wallpaper to" << fileName;

    if (image.save(fileName))
    {
        if (!setWallpaper(QDir::toNativeSeparators(fileName)))
        {
            QMessageBox::critical(this, windowTitle(), tr("Failed to set the wallpaper."));
        }
    }
    else
    {
        QMessageBox::critical(this, windowTitle(), tr("Failed to save the picture."));
    }

    close();
}


void MainWidget::paintCopyright(QImage &image) const
{
    QScreen *screen = QGuiApplication::primaryScreen();
    qreal ratio = image.height();
    ratio /= screen->size().height();
    qDebug() << "Ratio =" << ratio;

    QFont font = ui->fontCombo->currentFont();
    font.setPointSizeF(font.pointSizeF() * ratio);

    QFontMetricsF metrics(font);
    QRectF rect = metrics.boundingRect(copyright_);
    rect.setWidth(rect.width() * 1.5);

    const QSettings settings;
    int x = settings.value(QS("x"), 32).toInt();
    int y = settings.value(QS("y"), 32).toInt();
    rect.moveRight(image.width() - (x - 1) * ratio);
    rect.moveTop((y + 1) * ratio);

    QPainter painter(&image);
    painter.setFont(font);
    painter.setPen(Qt::black);
    painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter, copyright_);
    painter.setPen(QColor("whitesmoke"));
    painter.drawText(rect.translated(-ratio, -ratio), Qt::AlignRight | Qt::AlignVCenter, copyright_);
}
