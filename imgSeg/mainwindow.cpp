#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("8810 Preprocessor");
    passes = 1;
    histograms = new QLabel();
    dispScale = 1.0;
}

MainWindow::~MainWindow() {
    delete ui;
    delete histograms;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    Key key = Key(event->key());
    if (key >= Qt::Key_1 && key <= Qt::Key_4)
        view = key - Qt::Key_1;
    else if (key == Qt::Key_7 || key == Qt::Key_6)
        view = 4;
    if (key == Qt::Key_1)
        toDisplay = og;
    else if (key == Qt::Key_2)
        toDisplay = ogEd;
    else if (key == Qt::Key_3)
        toDisplay = edL;
    else if (key == Qt::Key_4)
        toDisplay = processed;
    else if (key == Qt::Key_5) {
        QInputDialog kerPrompt;
        kerPrompt.setOptions(QInputDialog::UseListViewForComboBoxItems);
        kerPrompt.setComboBoxItems(eTypes);
        kerPrompt.setTextValue(eTypes.first());
        kerPrompt.setWindowTitle("Histograms");
        int execCode = kerPrompt.exec();
        if (execCode == 0)
            return;
        int index = eTypes.indexOf(kerPrompt.textValue());
        histogram(eType(index));
        histograms->setWindowModality(Qt::ApplicationModal);
        histograms->show();
    }
    else if (key == Qt::Key_6) {
        if (processed.isNull())
            return;
        QInputDialog kerPrompt;
        kerPrompt.setOptions(QInputDialog::UseListViewForComboBoxItems);
        kerPrompt.setComboBoxItems(eTypes);
        kerPrompt.setTextValue(eTypes.first());
        kerPrompt.setWindowTitle("Color Space");
        int execCode = kerPrompt.exec();
        if (execCode == 0)
            return;
        sensitivityDialog sd(this);
        eType type = eType(eTypes.indexOf(kerPrompt.textValue()));
        sd.setWork(processed, type);
        execCode = sd.exec();
        if (execCode == 0)
            return;
        bool ok = false;
        int ret = QInputDialog::getInt(this, "8810", "kmean iterations to complete", 0, 0, 500, 1, &ok);
        if (ok)
            toDisplay = kmeans(sd.getPoints(), ret, type);
        QApplication::beep();
    }
    else if (key == Qt::Key_7)
        toDisplay = segmented;
    else if (key == Qt::Key_8 && !colors.empty()) {
        segCleaner sc(this);
        sc.setWork(&cleanSeg, cleanMat, colors);
        sc.exec();
        toDisplay = cleanSeg;
    }
    else if (key == Qt::Key_9)
        toDisplay = cleanSeg;
    else if (key == Qt::Key_Alt)
        toDisplay.save(filename + "_view_" + views[view] + "_" + to_string(passes).c_str() + ".png");
    else if (key == Qt::Key_Space) {
        compute();
        cleanSeg = segmented = processed.copy();
    }
    else if (key == Qt::Key_Control) {
        bool ok = false;
        int ret = QInputDialog::getInt(this, "8810", "Process cycles to complete", 0, 0, 100, 1, &ok);
        if (ok)
            for (int i = 0; i < ret; ++i) {
                ui->statusbar->showMessage(QString(to_string(passes).c_str()) + " cycles of " + QString(to_string(passes + (ret - i)).c_str()) + " completed");
                compute();
            }
        cleanSeg = segmented = processed.copy();
        for (int i = 0; i < 5; ++i)
            times[i] /= ret;
        cout << "avg initialization took " << times[0] << "ms" << endl;
        cout << "avg edge detection took " << times[1] << "ms" << endl;
        cout << "avg equalization took " << times[2] << "ms" << endl;
        cout << "avg trimming took " << times[3] << "ms" << endl;
        cout << "avg blurring took " << times[4] << "ms" << endl;
        cout << "batch complete" << endl;
        QApplication::beep();
    }
    else if (key == Qt::Key_Shift) {
        cout << threadCnt << endl;
        passes = 1;
        string formats = "";
        for (string s : acceptedImportImageFormats)
            formats += " *." + s;
        formats = "Image Files (" + formats.substr(1) + ")";
        QString fileName = QFileDialog::getOpenFileName(this, tr("Import"), "/", tr(formats.c_str()));
        if (fileName == "")
            return;
        string fn = fileName.toStdString();
        setWindowTitle("8810 Preprocessor - " + QString(fn.substr(fn.find_last_of('/') + 1).c_str()));
        size_t index = fn.find_last_of('.');
        filename = fn.substr(0, index).c_str();
        string fileType = fn.substr(index + 1);
        if (std::find(acceptedImportImageFormats.begin(), acceptedImportImageFormats.end(), fileType) != acceptedImportImageFormats.end())
        og = QImage(fileName).scaled(1920, 1080, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
        processed = og.copy();
        w = og.width();
        h = og.height();
        edL_p = iMat (w, vector<int>(h, 0));
        edL = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
        segmented = cleanSeg = edL;
        compute();
        toDisplay = og;
    }
    else if (key == Qt::Key_Left)
        dispScale -= 0.1f;
    else if (key == Qt::Key_Right)
        dispScale += 0.1f;
    ui->statusbar->showMessage(QString(to_string(passes).c_str()) + " cycles completed");
    repaint();
}

void MainWindow::paintEvent(QPaintEvent *event) {
    if (toDisplay.isNull())
        return;
    QPainter qp(this);
    qp.drawImage(0, 0, toDisplay.scaledToWidth(dispScale * toDisplay.width(), Qt::SmoothTransformation));
}

fMat MainWindow::getBoxBlur(int width) {
    float w = tof(width * width);
    vector<float> sub(width, 1.0 / w);
    return fMat (width, sub);
}

fMat MainWindow::getConeBlur(int width) {
    float r = tof(width) / 2.0;
    fMat vec(width, vector<float>(width, 0.0));
    float cnt = 0.0;
    for (int i = 0; i < width; ++i) {
        int I = i - toi(r);
        for (int j = 0; j < width; ++j) {
            int J = j - toi(r);
            float dist = sqrt(tof(I * I + J * J));
            if (dist <= r) {
                vec[i][j] = (r - dist);
                cnt += vec[i][j];
            }
        }
    }
    for (int i = 0; i < width; ++i)
        for (int j = 0; j < width; ++j)
            vec[i][j] /= cnt;
    return vec;
}

void MainWindow::compute() {
    if (og.isNull())
        return;
    int oShift = sqrt(passes);
    long long time;
    time = getTime(0);
    for (int j = 0; j < h; ++j) {
        QRgb *line = reinterpret_cast<QRgb *>(processed.scanLine(j));
        for (int i = 0; i < w; ++i) {
            QColor qc(line[i]);
            edL_p[i][j] = (qc.red() + qc.green() + qc.blue()) / 3;
        }
    }
    times[0] += getTime(time);
    //cout << "edL_p initialization took " << getTime(time) << "ms" << endl;
    time = getTime(0);
    for (int j = 0; j < h; ++j) {
        QRgb *line = reinterpret_cast<QRgb *>(edL.scanLine(j));
        for (int i = 0; i < w; ++i) {
            int totalL_1 = 0, totalL_2 = 0;
            for (int m = 0; m < 3; ++m)
                for (int n = 0; n < 3; ++n) {
                    if ((m - 1) + i < 0 || (n - 1) + j < 0 || (m - 1) + i >= w || (n - 1) + j >= h) {
                        totalL_1 += sobel[m][n] * edL_p[i][j];
                        totalL_2 += sobel[n][m] * edL_p[i][j];
                    }
                    else {
                        totalL_1 += sobel[m][n] * edL_p[(m - 1) + i][(n - 1) + j];
                        totalL_2 += sobel[n][m] * edL_p[(m - 1) + i][(n - 1) + j];
                    }
                }
            int lit = toi(sqrt(tod(totalL_1 * totalL_1 + totalL_2 * totalL_2)) * scale);
            lit = 255 - ((lit >> (oShift)) << (oShift));
            line[i] = 0xFF000000 | (lit << 16) | (lit << 8) | lit;
        }
    }
    times[1] += getTime(time);
    //cout << "edge detection took " << getTime(time) << "ms" << endl;
    time = getTime(0);
    edL = equalize(edL);
    times[2] += getTime(time);
    //cout << "equalization took " << getTime(time) << "ms" << endl;
    time = getTime(0);
    for (int j = 0; j < h; ++j) {
        QRgb *line = reinterpret_cast<QRgb *>(edL.scanLine(j));
        for (int i = 0; i < w; ++i)
            if (QColor(line[i]).red() < (256 >> oShift))
                line[i] = 0xFF000000;
    }
    times[3] += getTime(time);
    //cout << "trimming took " << getTime(time) << "ms" << endl;
    if (passes == 1)
        ogEd = edL.copy();
    time = getTime(0);
    work = processed.copy();
    vector <QThread *> threads;
    for (int i = 0; i < threadCnt; ++i)
        threads.push_back(QThread::create(applyBlur, processed, &work, edL, i, passes));
    for (QThread * thread : threads)
        thread->start();
    applyBlur(processed, &work, edL, -1, passes);
    for (int i = threadCnt - 1; i >= 0; --i)
        if (threads[i]->isRunning())
            threads[i]->wait();
    times[4] += getTime(time);
    //cout << "blurring took " << getTime(time) << "ms" << endl;
    processed = work.copy();
    ++passes;
    toDisplay = processed.copy();
    repaint();
    QApplication::processEvents();
    cout << "process complete. passes " << passes << endl;
}

void MainWindow::applyBlur(QImage processed, QImage *work, QImage edL, int tIndex, int passes) {
    int diff = min((3 * passes) / 2, hueDiff);
    int kWidth = 1 + 2 * (passes / 2), offset = kWidth / 2;
    fMat blur = getConeBlur(kWidth);
    int width = processed.width(), height = processed.height();
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
    for (int j = start; j < end; ++j) {
        QRgb *line = reinterpret_cast<QRgb *>(processed.scanLine(j));
        QRgb *line2 = reinterpret_cast<QRgb *>(work->scanLine(j));
        QRgb *line4 = reinterpret_cast<QRgb *>(edL.scanLine(j));
        for (int i = 0; i < width; ++i) {
            float rt = 0.0, gt = 0.0, bt = 0.0;
            QColor qc(line[i]), center = qc;
            int huePro = qc.hslHue() + 1;
            int satPro = qc.hslSaturation();
            int valPro = qc.lightness();
            for (int n = 0; n < kWidth; ++n) {
                QRgb *line3 = nullptr;
                int J = (n - offset) + j;
                if (J >= 0 && J < height)
                    line3 = reinterpret_cast<QRgb *>(work->scanLine((n - offset) + j));
                for (int m = 0; m < kWidth; ++m) {
                    if ((m - offset) + i < 0 || (m - offset) + i >= width || line3 == nullptr)
                        qc = center;
                    else {
                        qc = line3[(m - offset) + i];
                        int hue = qc.hslHue() + 1;
                        int sat = qc.hslSaturation();
                        int val = qc.lightness();
                        if (abs(huePro - hue) > 180)
                            hue -= 360;
                        if (abs(huePro - hue) <= diff && abs(satPro - sat) <= passes * satDiffMult && abs(valPro - val) <= passes * litDiffMult)// - passes)
                            qc = line3[(m - offset) + i];
                        else
                            qc = center;
                    }
                    rt += blur[m][n] * qc.redF();
                    gt += blur[m][n] * qc.greenF();
                    bt += blur[m][n] * qc.blueF();
                }
            }
            qc = center;
            QColor dL(line4[i]);
            float rd = dL.redF();
            rt = qc.redF() * (1.0 - rd) + rt * rd;
            float gd = dL.greenF();
            gt = qc.greenF() * (1.0 - gd) + gt * gd;
            float bd = dL.blueF();
            bt = qc.blueF() * (1.0 - bd) + bt * bd;
            qc.setRgbF(rt, gt, bt, qc.alphaF());
            line2[i] = qc.rgba();
        }
    }
}

QImage MainWindow::equalize(QImage qi) {
    int W = qi.width(), H = qi.height();
    //normalize into 0-255 range
    int histo[bins] = {0};
    // Fill the array(s) tht the histograms will be constructed from.
    int total = 0;
    for (int y = 0; y < H; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(qi.scanLine(y));
        for (int x = 0; x < W; ++x) {
            QColor qc(line[x]);
            if (qc.alpha() != 0) {
                int value = (qc.red() + qc.green() + qc.blue()) / 3;
                ++total;
                ++histo[value];
            }
        }
    }
    int i = 0;
    while (i < bins && histo[i] == 0)
        ++i;
    if (i == bins || histo[i] == total)
        return qi;
    float scale = tof(bins - 1) / tof(total - histo[i]);
    int lut[bins] = {0};
    int sum = 0;
    for (++i; i < bins; ++i) {
        sum += histo[i];
        lut[i] = toi(tof(sum) * scale);
    }
    for (int y = 0; y < H; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(qi.scanLine(y));
        for (int x = 0; x < W; ++x) {
            QColor qc(line[x]);
            if (qc.alpha() != 0)
                qi.setPixelColor(x, y, QColor(lut[qc.red()], lut[qc.green()], lut[qc.blue()]));
        }
    }
    return qi;
}

void MainWindow::histogram(eType type) {
    int H = 3 * bins;
    QImage qi (QSize(bins * 2, H), QImage::Format_ARGB32_Premultiplied);
    histograms->resize(qi.width(), qi.height());
    histograms->setWindowFilePath("Histogram");
    qi.fill(0xFF000000);
    int end = type == RGB ? 4 : 3;
    int histo[4][bins] = {{0}, {0}, {0}, {0}};
    QImage image = toDisplay.copy();
    // Fill the array(s) tht the histograms will be constructed from.
    int total = 0;
    for (int y = 0; y < image.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            QColor qc(line[x]);
            if (qc.alpha() != 0) {
                if (type == RGB) {
                    int intensity = toi(tof(qc.red() + qc.green() + qc.blue()) / 3.0 + 0.5);
                    ++histo[0][intensity];
                    ++histo[1][qc.red()];
                    ++histo[2][qc.green()];
                    ++histo[3][qc.blue()];
                }
                else if (type == HSV) {
                    ++histo[0][toi(255.0 * tof(qc.hsvHue() + 1.0) / 360.0)];
                    ++histo[1][toi(255.0 * qc.hsvSaturationF())];
                    ++histo[2][toi(255.0 * qc.valueF())];
                }
                else if (type == HSL) {
                    ++histo[0][toi(255.0 * tof(qc.hslHue() + 1.0) / 360.0)];
                    ++histo[1][toi(255.0 * qc.hslSaturationF())];
                    ++histo[2][toi(255.0 * qc.lightnessF())];
                }
                else if (type == LAB) {
                    vector <float> color = getLabScaled(rgb2lab(qc));
                    for (unsigned char c = 0; c < 3; ++c)
                        ++histo[c][toi(color[c])];
                }
                ++total;
            }
        }
    }
    int maxI = 0, cutoff = total / 4;
    for (int j = 0; j < end; ++j)
        for (int i = 1; i < bins - 1; ++i)
            if (histo[j][i] < cutoff)
                maxI = max(maxI, histo[j][i]);
    // Draw histograms.
    maxI = max(maxI, 1);
    double div = tod(H / 2 - 1) / tod(maxI);
    float fbins = tof(bins - 1);
    for (int x = 0; x < bins; ++x) {
        QRgb value = static_cast<QRgb>(bins + x) / 2;
        for  (int j = 0; j < end; ++j) {
            QRgb color = 0xFF000000;
            if (type == RGB) {
                if (j != 0)
                    color += (value << (8 * (2 - (j - 1))));
                else
                    for (unsigned char i = 0; i < 3; ++i)
                        color += value << (8 * i);
            }
            else if (type == HSV) {
                QColor qc(color);
                if (j == 0) {
                    qc.setHsvF(tof(x) / fbins, 1.0f, 1.0f);
                    color = qc.rgba();
                }
                else if (j == 1) {
                    qc.setHsvF(0.3f, tof(x) / fbins, 1.0f);
                    color = qc.rgba();
                }
                else
                    for (unsigned char i= 0; i < 3; ++i)
                        color += value << (8 * i);
            }
            else if (type == HSL) {
                QColor qc(color);
                if (j == 0) {
                    qc.setHslF(tof(x) / fbins, 1.0, 0.5);
                    color = qc.rgba();
                }
                else if (j == 1) {
                    qc.setHslF(0.3f, tof(x) / fbins, 0.5f);
                    color = qc.rgba();
                }
                else
                    for (unsigned char i= 0; i < 3; ++i)
                        color += value << (8 * i);
            }
            else if (type == LAB) {
                if (j == 0)
                    for (unsigned char i= 0; i < 3; ++i)
                        color += value << (8 * i);
                else if (j == 1)
                    color = QColor(255 - x, x, 0).rgba();
                else
                    color = QColor(255 - x, 255 - x, x).rgba();
            }
            int rowOffset = (H / 2) * (j / 2);
            int colOffset = x + bins * (j % 2);
            int stop = H / 2 + rowOffset;
            int start = stop - toi(tod(histo[j][x]) * div);
            start = max(start, rowOffset);
            for (int y = start; y < stop; ++y)
                qi.setPixelColor(colOffset, y, color);
        }
    }
    histograms->setPixmap(QPixmap::fromImage(qi));
    histograms->setFixedSize(histograms->size());
}

QImage MainWindow::kmeans(vector <QColor> centers, int num, eType type) {
    if (num == 0)
        return segmented;
    long long t0, t1 = 0, t2 = 0, t3 = getTime(0);
    segmented = QImage(processed.size(), QImage::Format_ARGB32_Premultiplied);
    int w = processed.width(), h = processed.height();
    vector <QColor> clusterCenters(centers);
    fMat labClusterCenters;
    vector <fMat> labImage;
    void *image = type == LAB ? (void *)(&labImage) : (void *)(&processed);
    void *cc = type == LAB ? (void *)(&labClusterCenters) : (void *)(&clusterCenters);
    vector <QThread *> threads;
    if (type == LAB) {
        for (QColor qc : centers)
            labClusterCenters.push_back(rgb2lab(qc));
        labImage.resize(w, fMat(h, vector<float>(3, 0.0)));
        for (int i = 0; i < threadCnt; ++i)
            threads.push_back(QThread::create(toLab, &labImage, processed, i));
        for (QThread * thread : threads)
            thread->start();
        toLab(&labImage, processed, -1);
        for (int i = threadCnt - 1; i >= 0; --i)
            if (threads[i]->isRunning())
                threads[i]->wait();
    }
    int K = toi(centers.size());
    cMat labels(w, vector<unsigned char>(h, 0));
    t0 = getTime(0);
    getDistances(image, &labels, cc, type);
    t1 += getTime(t0);
    for (int i = 0; i < num; ++i) {
        t0 = getTime();
        for (int k = 0; k < K; ++k) {
            long mean_b = 0, mean_g = 0, mean_r = 0;
            int total = 0;
            int shift;
            if (type == HSV || type == HSL)
                shift = 180 - (clusterCenters[k].hsvHue() + 1);
            for (int y = 0; y < h; ++y) {
                QRgb *line = type == LAB ? nullptr : reinterpret_cast<QRgb *>(processed.scanLine(y));
                for (int x = 0; x < w; ++x) {
                    if (labels[x][y] == k) {
                        if (type == RGB) {
                            QColor qc(line[x]);
                            mean_r += qc.red();
                            mean_g += qc.green();
                            mean_b += qc.blue();
                        }
                        else if (type == HSV) {
                            QColor qc(line[x]);
                            int hue = (qc.hsvHue() + 1) + shift;
                            if (hue > 360)
                                hue -= 360;
                            else if (hue < 0)
                                hue += 360;
                            mean_r += hue;
                            mean_g += qc.hsvSaturation();
                            mean_b += qc.value();
                        }
                        else if (type == HSL) {
                            QColor qc(line[x]);
                            int hue = (qc.hslHue() + 1) + shift;
                            if (hue > 360)
                                hue -= 360;
                            else if (hue < 0)
                                hue += 360;
                            mean_r += hue;
                            mean_g += qc.hslSaturation();
                            mean_b += qc.lightness();
                        }
                        else if (type == LAB) {
                            vector <float> lab = labImage[x][y];
                            mean_r += lab[0];
                            mean_g += lab[1];
                            mean_b += lab[2];
                        }
                        ++total;
                    }
                }
            }
            if (total != 0) {
                mean_r /= total;
                mean_g /= total;
                mean_b /= total;
                if (type == RGB)
                    clusterCenters[k] = QColor(toi(mean_r), toi(mean_g), toi(mean_b));
                else if (type == HSV) {
                    QColor qc;
                    int hue = toi(mean_r) - shift;
                    if (hue > 360)
                        hue -= 360;
                    else if (hue < 0)
                        hue += 360;
                    qc.setHsv(hue - 1, mean_g, mean_b);
                    clusterCenters[k] = qc;
                }
                else if (type == HSL) {
                    QColor qc;
                    int hue = toi(mean_r) - shift;
                    if (hue > 360)
                        hue -= 360;
                    else if (hue < 0)
                        hue += 360;
                    qc.setHsl(hue - 1, mean_g, mean_b);
                    clusterCenters[k] = qc;
                }
                else if (type == LAB) {
                    labClusterCenters[k][0] = mean_r;
                    labClusterCenters[k][1] = mean_g;
                    labClusterCenters[k][2] = mean_b;
                }
            }
        }
        t2 += getTime(t0);
        t0 = getTime(0);
        getDistances(image, &labels, cc, type);
        t1 += getTime(t0);
        ui->statusbar->showMessage(QString(to_string(i).c_str()) + " iterations of " + QString(to_string(num).c_str()) + " completed");
        void * clusters = type == LAB ? (void *)(&labClusterCenters) : (void *)(&clusterCenters);
        toRGB(&toDisplay, labels, clusters, type);
        repaint();
        QApplication::processEvents();
    }
    void * clusters = type == LAB ? (void *)(&labClusterCenters) : (void *)(&clusterCenters);
    colors = toRGB(&segmented, labels, clusters, type);
    cleanMat = labels;
    t1 /= (K + 1);
    t2 /= K;
    cout << "Average Distancing Time: " << t1 << "ms" << endl;
    cout << "Average Meaning Time: " << t2 << "ms" << endl;
    cout << "Total kmeans time: " << getTime(t3) << "ms" << endl;
    return segmented;
}

void MainWindow::getDistances(void *image, cMat *labels, void *cc, eType type) {
    vector <QThread *> threads;
    for (int i = 0; i < threadCnt; ++i)
        threads.push_back(QThread::create(calcColorDistances, image, labels, cc, type, i));
    for (QThread * thread : threads)
        thread->start();
    calcColorDistances(image, labels, cc, type, -1);
    for (int i = threadCnt - 1; i >= 0; --i)
        if (threads[i]->isRunning())
            threads[i]->wait();
}

void MainWindow::calcColorDistances(void * image, cMat *labels, void * cc, eType type, int tIndex) {
    QImage Processed;
    vector <fMat> labImage;
    vector <QColor> clusterCenters;
    fMat labClusterCenters;
    int width, height, K;
    if (type == LAB) {
        labImage = *(vector<fMat> *)(image);
        labClusterCenters = *(fMat *)(cc);
        width = toi(labImage.size());
        height = toi(labImage[0].size());
        K = toi(labClusterCenters.size());
    }
    else {
        Processed = *(QImage *)(image);
        clusterCenters = *(vector <QColor> *)(cc);
        width = Processed.width();
        height = Processed.height();
        K = toi(clusterCenters.size());
    }
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
    if (type == LAB) {
        for (int y = start; y < end; ++y)
            for (int x = 0; x < width; ++x) {
                double min_dist = colorDistance(labClusterCenters[0], labImage[x][y]);
                for (int k = 1; k < K; ++k) {
                    double distance = colorDistance(labClusterCenters[k], labImage[x][y]);
                    if (distance < min_dist) {
                        min_dist = distance;
                        (*labels)[x][y] = toc(k);
                    }
                }
            }
    }
    else {
        for (int y = start; y < end; ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(Processed.scanLine(y));
            for (int x = 0; x < width; ++x) {
                QColor qc(line[x]);
                double min_dist = colorDistance(clusterCenters[0], qc, type);
                for (int k = 1; k < K; ++k) {
                    double distance = colorDistance(clusterCenters[k], qc, type);
                    if (distance < min_dist) {
                        min_dist = distance;
                        (*labels)[x][y] = toc(k);
                    }
                }
            }
        }
    }
}

double MainWindow::colorDistance(vector <float> m, vector <float> n) {
    const double k_L = 1.0, k_C = 1.0, k_H = 1.0;
    const double deg360InRad = 2.0 * pi;
    const double deg180InRad = pi;
    const double pow25To7 = 6103515625.0;
    double C1 = sqrt((m[1] * m[1]) + (m[1] * m[1]));
    double C2 = sqrt((n[1] * n[1]) + (n[2] * n[2]));
    double barC = (C1 + C2) / 2.0;
    double G = 0.5 * (1 - sqrt(pow(barC, 7) / (pow(barC, 7) + pow25To7)));
    double a1Prime = (1.0 + G) * m[1];
    double a2Prime = (1.0 + G) * n[1];
    double CPrime1 = sqrt((a1Prime * a1Prime) + (m[2] * m[2]));
    double CPrime2 = sqrt((a2Prime * a2Prime) + (n[2] * n[2]));
    double hPrime1;
    if (m[2] == 0 && a1Prime == 0)
        hPrime1 = 0.0;
    else {
        hPrime1 = atan2(m[2], a1Prime);
        if (hPrime1 < 0)
            hPrime1 += deg360InRad;
    }
    double hPrime2;
    if (n[2] == 0 && a2Prime == 0)
        hPrime2 = 0.0;
    else {
        hPrime2 = atan2(n[2], a2Prime);
        if (hPrime2 < 0)
            hPrime2 += deg360InRad;
    }
    double deltaLPrime = n[0] - m[0];
    double deltaCPrime = CPrime2 - CPrime1;
    double deltahPrime;
    double CPrimeProduct = CPrime1 * CPrime2;
    if (CPrimeProduct == 0)
        deltahPrime = 0;
    else {
        deltahPrime = hPrime2 - hPrime1;
        if (deltahPrime < -deg180InRad)
            deltahPrime += deg360InRad;
        else if (deltahPrime > deg180InRad)
            deltahPrime -= deg360InRad;
    }
    double deltaHPrime = 2.0 * sqrt(CPrimeProduct) * sin(deltahPrime / 2.0);
    double barLPrime = (m[0] + n[0]) / 2.0;
    double barCPrime = (CPrime1 + CPrime2) / 2.0;
    double barhPrime, hPrimeSum = hPrime1 + hPrime2;
    if (CPrime1 * CPrime2 == 0) {
        barhPrime = hPrimeSum;
    } else {
        if (fabs(hPrime1 - hPrime2) <= deg180InRad)
            barhPrime = hPrimeSum / 2.0;
        else {
            if (hPrimeSum < deg360InRad)
                barhPrime = (hPrimeSum + deg360InRad) / 2.0;
            else
                barhPrime = (hPrimeSum - deg360InRad) / 2.0;
        }
    }
    double rad30 = (30.0 / 180.0) * pi;
    double T = 1.0 - (0.17 * cos(barhPrime - rad30)) + (0.24 * cos(2.0 * barhPrime)) + (0.32 * cos((3.0 * barhPrime) + (6.0 / 180.0) * pi)) - (0.20 * cos((4.0 * barhPrime) - (63.0 / 180.0) * pi));
    double deltaTheta = rad30 * exp(-pow((barhPrime - (275.0 / 180.0) * pi) / ((25.0 / 180.0) * pi), 2.0));
    double R_C = 2.0 * sqrt(pow(barCPrime, 7.0) / (pow(barCPrime, 7.0) + pow25To7));
    double S_L = 1 + ((0.015 * pow(barLPrime - 50.0, 2.0)) / sqrt(20 + pow(barLPrime - 50.0, 2.0)));
    double S_C = 1 + (0.045 * barCPrime);
    double S_H = 1 + (0.015 * barCPrime * T);
    double R_T = (-sin(2.0 * deltaTheta)) * R_C;
    double deltaE = sqrt(pow(deltaLPrime / (k_L * S_L), 2.0) + pow(deltaCPrime / (k_C * S_C), 2.0) + pow(deltaHPrime / (k_H * S_H), 2.0) + (R_T * (deltaCPrime / (k_C * S_C)) * (deltaHPrime / (k_H * S_H))));
    return deltaE;
}

int MainWindow::colorDistance(QColor m, QColor n, eType type) {
    int a = 0, b = 0, c = 0;
    if (type == RGB) {
        a = m.red() - n.red();
        b = m.green() - n.green();
        c = m.blue() - n.blue();
    }
    else if (type == HSV) {
        a = abs(m.hsvHue() - n.hsvHue());
        if (a >= 180)
            a = 360 - a;
        b = m.hsvSaturation() - n.hsvSaturation();
        c = m.value() - n.value();
    }
    else if (type == HSL) {
        a = abs(m.hslHue() - n.hslHue());
        if (a >= 180)
            a = 360 - a;
        b = m.hslSaturation() - n.hslSaturation();
        c = m.lightness() - n.lightness();
    }
    return pow(a, 2) + pow(b, 2) + pow(c, 2);
}

void MainWindow::toLab(vector <fMat> *lab, QImage rgb, int tIndex) {
    int width = rgb.width(), height = rgb.height();
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
        QRgb *line = reinterpret_cast<QRgb *>(rgb.scanLine(y));
        for (int x = 0; x < width; ++x)
            (*lab)[x][y] = rgb2lab(line[x]);
    }
}

vector <QRgb> MainWindow::toRGB(QImage *out, cMat labels, void *clusters, eType type) {
    vector <QThread *> threads;
    vector <QRgb> colors;
    if (type == LAB) {
        fMat labClusterCenters = *(fMat *)(clusters);
        for (vector <float> c : labClusterCenters)
            colors.push_back(lab2rgb(c).rgba());
    }
    else {
        vector <QColor> clusterCenters = *(vector <QColor> *)(clusters);
        for (QColor qc : clusterCenters)
            colors.push_back(qc.rgba());
    }
    for (int i = 0; i < threadCnt; ++i)
        threads.push_back(QThread::create(convert, out, labels, colors, i));
    for (QThread * thread : threads)
        thread->start();
    convert(out, labels, colors, -1);
    for (int i = threadCnt - 1; i >= 0; --i)
        if (threads[i]->isRunning())
            threads[i]->wait();
    return colors;
}

void MainWindow::convert(QImage *out, cMat labels, vector <QRgb> colors, int tIndex) {
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



