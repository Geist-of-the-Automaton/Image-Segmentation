#include "segcleaner.h"
#include "ui_segcleaner.h"

segCleaner::segCleaner(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::segCleaner)
{
    ui->setupUi(this);
    setWindowTitle("Segmentation Cleaner");
    val = 0.0;
}

segCleaner::~segCleaner() {
    delete ui;
}

void segCleaner::setWork(QImage *toProcess, cMat lbls, vector <QRgb> clrs) {
    qi = toProcess;
    labels = lbls;
    colors = clrs;
    w = qi->width();
    h = qi->height();
    iMat areas (w, vector<int>(h, 0));
    int currArea = 0;
    for (short i = 0; i < w; ++i)
        for (short j = 0; j < h; ++j)
            if (areas[i][j] == 0) {
                ++currArea;
                list <QPoint> areaPoints;
                list <QPoint> stack;
                areas[i][j] = currArea;
                stack.push_back(QPoint(i, j));
                int size = 0;
                unsigned char label = labels[i][j];
                while (!stack.empty()) {
                    areaPoints.push_back(stack.front());
                    short X = stack.front().x();
                    short Y = stack.front().y();
                    stack.pop_front();
                    ++size;
                    for (short m = 0; m < 3; ++m) {
                        short x = X + (m - 1);
                        if (x >= 0 && x < w)
                            for (short n = (m + 1) % 2; n < 3; n += 2) {
                                short y = Y + (n - 1);
                                if (y >= 0 && y < h && labels[x][y] == label && areas[x][y] == 0) {
                                    areas[x][y] = currArea;
                                    stack.push_front(QPoint(x, y));
                                }
                            }
                    }
                }
                points.push_back(areaPoints);
                sizes.push_back(size);
                //cout << "area " << currArea << "with size " << size << endl;
            }
    for (int i = 0; i < points.size(); ++i) {
        vector <int> parents(sizes.size(), 0);
        for (QPoint p : points[i]) {
            short X = p.x();
            short Y = p.y();
            for (short m = 0; m < 3; ++m) {
                short x = X + (m - 1);
                if (x >= 0 && x < w)
                    for (short n = (m + 1) % 2; n < 3; n += 2) {
                        short y = Y + (n - 1);
                        if (y >= 0 && y < h && areas[x][y] != areas[X][Y])
                            ++parents[areas[x][y] - 1];
                    }
            }
        }
        int index = i;
        for (int j = 0; j < parents.size(); ++j)
            if (parents[j] > parents[index] || (parents[j] == parents[index] && sizes[j] > sizes[index]))
                index = j;
        parentsList.push_back(index);
    }
    process();
}

void segCleaner::on_spinbox_valueChanged(double value) {
    if (lock.try_lock()) {
        val = value;
        ui->slider->setValue(toi(1000.0 * value));
        process();
        lock.unlock();
    }
}

void segCleaner::on_slider_valueChanged(int value) {
    if (lock.try_lock()) {
        val = tod(value) / 1000.0;
        ui->spinbox->setValue(val);
        process();
        lock.unlock();
    }
}

void segCleaner::process() {
    double p = val / 1000.0;
    int len = toi(sizes.size());
    double totalSize = tod(w * h);
    cMat labelsFinal(labels);
    for (int i = 0; i < len; ++i) {
        int I = i;
        while (tod(sizes[I]) / totalSize < p) {
            if (I == parentsList[I] || I == parentsList[parentsList[I]])
                break;
            I = parentsList[I];
        }
        short x = points[I].front().x();
        short y = points[I].front().y();
        unsigned char c = labels[x][y];
        for (QPoint p : points[i])
            labelsFinal[p.x()][p.y()] = c;
    }
    vector <QThread *> threads;
    for (int i = 0; i < threadCnt; ++i)
            threads.push_back(QThread::create(processThreaded, qi, labelsFinal, colors, i));
        for (QThread * thread : threads)
            thread->start();
        processThreaded(qi, labelsFinal, colors, -1);
        for (int i = threadCnt - 1; i >= 0; --i)
            if (threads[i]->isRunning())
                threads[i]->wait();
    ui->label->setPixmap(QPixmap::fromImage(*qi));
}

void segCleaner::processThreaded(QImage *out, cMat labels, vector <QRgb> colors, int tIndex) {
    int width = out->width(), height = out->height();
    int lines = height / threadCnt;
    int start, end;
    if (tIndex == -1) {
        start = threadCnt * lines;
        end = height;
    }
    else {
        start = tIndex * lines;
        end = start + lines;
    }
    for (int y = start; y < end; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(out->scanLine(y));
        for (int x = 0; x < width; ++x)
            line[x] = colors[labels[x][y]];
    }
}

void segCleaner::on_buttonBox_accepted() {
    done(1);
}

void segCleaner::on_buttonBox_rejected() {
    done(0);
}


