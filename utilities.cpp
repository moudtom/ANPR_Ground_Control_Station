#include "utilities.h"
#include <QObject>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>


QString Utilities::getDataPath()
{
    QString user_pictures_path = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0];
    QDir pictures_dir(user_pictures_path);
    pictures_dir.mkpath("ANPR");
    return pictures_dir.absoluteFilePath("ANPR");
}

QString Utilities::newPhotoName()
{
    QDateTime time = QDateTime::currentDateTime();
    return time.toString("yyyy_MM_dd+HH.mm.ss");
}

QString Utilities::getPhotoPath(QString name, QString postfix)
{
    return QString("%1/%2.%3").arg(Utilities::getDataPath(), name, postfix);
}

QString Utilities::newdateName()
{
    QDateTime time = QDateTime::currentDateTime();
    return time.toString("yyyy_MM_dd");
}

QString Utilities::newtimeName()
{
    QDateTime time = QDateTime::currentDateTime();
    return time.toString("HH.mm.ss");
}
