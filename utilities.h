#ifndef UTILITIES_H
#define UTILITIES_H

#include <QString>
class Utilities
{
public:
    static QString getDataPath();
    static QString newPhotoName();
    static QString getPhotoPath(QString name, QString postfix);

    static QString newdateName();
    static QString newtimeName();
};

#endif // UTILITIES_H
