#ifndef SENSITIVITYDIALOG_H
#define SENSITIVITYDIALOG_H

#include <QDialog>
#include <QPainter>
#include <predefs.h>

namespace Ui {
class sensitivityDialog;
}

class sensitivityDialog : public QDialog
{
        Q_OBJECT
public:
    sensitivityDialog(QWidget *parent = nullptr);
    ~sensitivityDialog();
    void setWork(QImage toProcess, eType t);
    vector<QColor> getPoints();

private slots:
    void on_spinbox_valueChanged(double value);
    void on_slider_valueChanged(int value);
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    void process();

    Ui::sensitivityDialog *ui;
    QImage qi;
    double val;
    vector <QColor> centers;
    int lock;
    long long t1, t2, count;
    eType type;

};

#endif // SENSITIVITYDIALOG_H
