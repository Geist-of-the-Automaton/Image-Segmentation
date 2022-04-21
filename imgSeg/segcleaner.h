#ifndef SEGCLEANER_H
#define SEGCLEANER_H

#include <QDialog>
#include <QPainter>
#include <predefs.h>
#include <mutex>

using std::mutex;


namespace Ui {
class segCleaner;
}

class segCleaner : public QDialog
{
        Q_OBJECT
public:
    segCleaner(QWidget *parent = nullptr);
    ~segCleaner();
    void setWork(QImage *toProcess, cMat lbls, vector <QRgb> clrs);

private slots:
    void on_spinbox_valueChanged(double value);
    void on_slider_valueChanged(int value);
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    void process();
    static void processThreaded(QImage *out, cMat labels, vector <QRgb> colors, int tIndex);

    Ui::segCleaner *ui;
    QImage *qi;
    double val;
    int w, h;
    cMat labels;
    vector <QRgb> colors;
    vector <int> sizes;
    vector <int> parentsList;
    vector<list<QPoint>> points;

    mutex lock;

};

#endif // SEGCLEANER_H
