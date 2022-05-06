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
#include <segcleaner.h>
#include <predefs.h>

static long long times[5] = {0};

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
    void getDistances(void * image, cMat *labels, void * cc, eType type);
    static int colorDistance(QColor m, QColor n, eType type);
    static double colorDistance(vector <float> m, vector <float>);
    static void calcColorDistances(void * image, cMat *labels, void * cc, eType type, int tIndex);
    static void toLab(vector <fMat> *lab, QImage rgb, int tIndex);
    static vector <QRgb> toRGB(QImage *out, cMat labels, void *clusters, eType type);
    static void convert(QImage *out, cMat labels, vector <QRgb> colors, int tIndex);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QImage og, edL, ogEd, combo, processed, toDisplay, work, segmented, cleanSeg;
    QLabel *histograms;
    int passes;
    QString filename;
    iMat edL_p;
    int w, h;
    int view;
    float dispScale;
    cMat cleanMat;
    vector <QRgb> colors;
};
#endif // MAINWINDOW_H
