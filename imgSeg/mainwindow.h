#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QColor>
#include <QFileDialog>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QInputDialog>
#include <QLabel>
#include <QThread>

#include <predefs.h>

#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <list>

using Qt::Key;
using std::cout;
using std::endl;
using std::vector;
using std::max;
using std::min;
using std::string;
using std::to_string;
using std::list;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void keyPressEvent(QKeyEvent *event);
    void paintEvent(QPaintEvent *event);
    static vector< vector<float> > getBoxBlur(int width);
    static vector< vector<float> > getConeBlur(int width);
    void compute();
    QImage equalize(QImage qi);
    void histogram(eType type);
    static void applyBlur(QImage processed, QImage *work, QImage edL, int tIndex, int passes);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QImage og, edL, ogEd, combo, processed, toDisplay, work;
    QLabel *histograms;
    int passes;
    QString filename;
    vector< vector<int> > sobel;
    vector< vector<int> > edL_p;
    int w, h;
    int view;
};
#endif // MAINWINDOW_H
