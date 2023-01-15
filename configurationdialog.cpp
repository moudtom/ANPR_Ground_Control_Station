#include "configurationdialog.h"
#include "ui_configurationdialog.h"

ConfigurationDialog::ConfigurationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigurationDialog)
{
    ui->setupUi(this);
    ui->lineEdit_w->setText("250");
    ui->lineEdit_h->setText("100");
    ui->lineEdit_threshold->setText("180");
    ui->lineEdit_contrast->setText("2.5");


}

ConfigurationDialog::~ConfigurationDialog()
{
    delete ui;
}

void ConfigurationDialog::setplateWidth(const QString &p1name)
{
    ui->lineEdit_w->setText(p1name);
}

void ConfigurationDialog::setplateHeight(const QString &p2name)
{
    ui->lineEdit_h->setText(p2name);
}

void ConfigurationDialog::setplateThrshold(const QString &p3name)
{
    ui->lineEdit_threshold->setText(p3name);
}

void ConfigurationDialog::setplatecontrast(const QString &p4name)
{
   ui->lineEdit_contrast->setText(p4name);
}

QString ConfigurationDialog::plateWidth() const
{
    return ui->lineEdit_w->text();

}

QString ConfigurationDialog::plateHeight() const
{
    return ui->lineEdit_h->text();

}

QString ConfigurationDialog::plateThreshold() const
{
    return ui->lineEdit_threshold->text();
}

QString ConfigurationDialog::plateContrast() const
{
   return ui->lineEdit_contrast->text();
}

void ConfigurationDialog::updateOKButtonState()
{
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
//    okButton->setEnabled(!ui->player1Name->text().isEmpty() &&
//                         !ui->player2Name->text().isEmpty());


}
