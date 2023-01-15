#ifndef CONFIGURATIONDIALOG_H
#define CONFIGURATIONDIALOG_H

#include <QDialog>

namespace Ui {
class ConfigurationDialog;
}

class ConfigurationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigurationDialog(QWidget *parent = nullptr);
    ~ConfigurationDialog();
    void setplateWidth(const QString &p1name);
    void setplateHeight(const QString &p2name);
    void setplateThrshold(const QString &p3name);
    void setplatecontrast(const QString &p4name);
    QString plateWidth() const;
    QString plateHeight() const;
    QString plateThreshold() const;
    QString plateContrast() const;
private slots:
    void updateOKButtonState();
private:
    Ui::ConfigurationDialog *ui;
};

#endif // CONFIGURATIONDIALOG_H
