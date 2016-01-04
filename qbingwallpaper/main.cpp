#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <QGuiApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>


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
    qDebug() << font;
    
    QFontMetricsF metrics(font);
    QRectF rect = metrics.boundingRect(copyright);
    rect.setWidth(rect.width() * 1.5);

    int x = settings.value(QS("x"), 32).toInt();
    int y = settings.value(QS("y"), 32).toInt();
    rect.moveRight(image->width() - (x - 1) * ratio);
    rect.moveTop((y + 1) * ratio);
    qDebug() << rect;

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


static void setWallpaper(const QString &fileName)
{
#ifdef Q_OS_WIN
    WCHAR wszFileName[MAX_PATH] = { 0 };
    fileName.toWCharArray(wszFileName);
    SystemParametersInfoW(SPI_SETDESKWALLPAPER, static_cast<UINT>(wcslen(wszFileName)), wszFileName,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
#endif
}


static void handleImageReply(QNetworkReply *reply, const QString &copyright)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        QImage image = QImage::fromData(reply->readAll());
        paintCopyright(&image, copyright);

        QFileInfo info(reply->url().fileName());
        QDir dir(outputDirectory());
        QString fileName = dir.absoluteFilePath(info.completeBaseName() % QS(".png"));

        if (image.save(fileName))
        {
            setWallpaper(QDir::toNativeSeparators(fileName));
        }
        else
        {
            qCritical() << "Failed to save image.";
        }
    }

    reply->deleteLater();
    QGuiApplication::quit();
}


static void handleArchiveReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        Q_ASSERT(doc.isObject());

        QJsonObject obj = doc.object();
        Q_ASSERT(obj.contains(QS("images")));

        QJsonValue value = obj.value(QS("images"));
        Q_ASSERT(value.isArray());

        obj = value.toArray().first().toObject();
        QString copyright = obj.value("copyright").toString();
        QString url = QS("https://www.bing.com") % obj.value(QS("url")).toString();
        qDebug() << copyright;
        qDebug() << url;

        QNetworkAccessManager *manager = reply->manager();
        QNetworkReply *r = manager->get(QNetworkRequest(QUrl(url)));
        QObject::connect(r, &QNetworkReply::finished, [r, copyright]()
        {
            handleImageReply(r, copyright);
        });
    }

    reply->deleteLater();
}


int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    QGuiApplication::setApplicationName(QS("qbingwallpaper"));
    QGuiApplication::setApplicationDisplayName(QS("QBingWallpaper"));
    QGuiApplication::setOrganizationName(QS("Roy QIU"));
    QGuiApplication::setOrganizationDomain(QS("karoyqiu.com"));


    QNetworkAccessManager manager;
    manager.setProxy(QNetworkProxy::NoProxy);

    qDebug() << "Getting wallpaper's URL...";
    QNetworkRequest request(QUrl(QS("https://www.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1")));
    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, [reply]()
    {
        handleArchiveReply(reply);
    });

    return a.exec();
}
