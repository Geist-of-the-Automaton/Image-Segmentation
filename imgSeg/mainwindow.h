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

#include <sensitivitydialog.h>
#include <predefs.h>

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
    static fMat getBoxBlur(int width);
    static fMat getConeBlur(int width);
    void compute();
    QImage equalize(QImage qi);
    void histogram(eType type);
    static void applyBlur(QImage processed, QImage *work, QImage edL, int tIndex, int passes);
    QImage kmeans(vector <QColor> centers, int iterations, eType type);
    static int colorDistance(QColor m, QColor n, eType type);
    static vector <float> rgb2lab (QColor qc);
    static QColor lab2rgb(vector <float> lab);
    static vector <float> getLabScaled(vector <float> lab);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QImage og, edL, ogEd, combo, processed, toDisplay, work, segmented;
    QLabel *histograms;
    int passes;
    QString filename;
    iMat edL_p;
    int w, h;
    int view;
    float dispScale;
};
#endif // MAINWINDOW_H
