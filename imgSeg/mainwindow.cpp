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
        sd.exec();
        bool ok = false;
        int ret = QInputDialog::getInt(this, "8810", "kmean iterations to complete", 0, 0, 500, 1, &ok);
        if (ok)
            toDisplay = kmeans(sd.getPoints(), ret, type);
        QApplication::beep();
    }
    else if (key == Qt::Key_7)
        toDisplay = segmented;
    else if (key == Qt::Key_Alt)
        toDisplay.save(filename + "_view_" + views[view] + "_" + to_string(passes).c_str() + ".png");
    else if (key == Qt::Key_Space)
        compute();
    else if (key == Qt::Key_Control) {
        bool ok = false;
        int ret = QInputDialog::getInt(this, "8810", "Process cycles to complete", 0, 0, 100, 1, &ok);
        if (ok)
            for (int i = 0; i < ret; ++i) {
                ui->statusbar->showMessage(QString(to_string(passes).c_str()) + " cycles of " + QString(to_string(passes + (ret - i)).c_str()) + " completed");
                compute();
            }
        cout << "batch complete" << endl;
        QApplication::beep();
    }
    else if (key == Qt::Key_Shift) {
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
        og = QImage(fileName);
        processed = og.copy();
        w = og.width();
        h = og.height();
        edL_p = iMat (w, vector<int>(h, 0));
        edL = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
        compute();
        toDisplay = og;
    }
    else if (key == Qt::Key_Left)
        dispScale -= 0.1;
    else if (key == Qt::Key_Right)
        dispScale += 0.1;
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
    float w = static_cast<float>(width * width);
    vector<float> sub(width, 1.0 / w);
    return fMat (width, sub);
}

fMat MainWindow::getConeBlur(int width) {
    float r = static_cast<float>(width) / 2.0;
    fMat vec(width, vector<float>(width, 0.0));
    float cnt = 0.0;
    for (int i = 0; i < width; ++i) {
        int I = i - static_cast<int>(r);
        for (int j = 0; j < width; ++j) {
            int J = j - static_cast<int>(r);
            float dist = sqrt(static_cast<float>(I * I + J * J));
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
    if (passes >= 1) {
        time = getTime(0);
        for (int j = 0; j < h; ++j) {
            QRgb *line = reinterpret_cast<QRgb *>(processed.scanLine(j));
            for (int i = 0; i < w; ++i) {
                QColor qc(line[i]);
                edL_p[i][j] = (qc.red() + qc.green() + qc.blue()) / 3;
            }
        }
        cout << "edL_p initialization took " << getTime(time) << "ms" << endl;
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
                int lit = static_cast<int>(sqrt(static_cast<double>(totalL_1 * totalL_1 + totalL_2 * totalL_2)) * scale);
                lit = 255 - ((lit >> (oShift)) << (oShift));
                line[i] = 0xFF000000 | (lit << 16) | (lit << 8) | lit;
            }
        }
        cout << "edge detection took " << getTime(time) << "ms" << endl;
        time = getTime(0);
        edL = equalize(edL);
        cout << "equalization took " << getTime(time) << "ms" << endl;
        time = getTime(0);
        for (int j = 0; j < h; ++j) {
            QRgb *line = reinterpret_cast<QRgb *>(edL.scanLine(j));
            for (int i = 0; i < w; ++i)
                if (QColor(line[i]).red() < (256 >> oShift))
                    line[i] = 0xFF000000;
        }
        cout << "trimming took " << getTime(time) << "ms" << endl;
    }
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
    cout << "blurring took " << getTime(time) << "ms" << endl;
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
    float scale = static_cast<double>(bins - 1) / static_cast<double>(total - histo[i]);
    int lut[bins] = {0};
    int sum = 0;
    for (++i; i < bins; ++i) {
        sum += histo[i];
        lut[i] = static_cast<int>(static_cast<float>(sum) * scale);
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
    for (int x = 0; x < image.width(); ++x)
        for (int y = 0; y < image.height(); ++y) {
            QColor qc = image.pixelColor(x, y);
            if (qc.alpha() != 0) {
                if (type == RGB) {
                    int intensity = static_cast<int>(static_cast<float>(qc.red() + qc.green() + qc.blue()) / 3.0 + 0.5);
                    ++histo[0][intensity];
                    ++histo[1][qc.red()];
                    ++histo[2][qc.green()];
                    ++histo[3][qc.blue()];
                }
                else if (type == HSV) {
                    ++histo[0][static_cast<int>(255.0 * static_cast<float>(qc.hsvHue() + 1.0) / 360.0)];
                    ++histo[1][static_cast<int>(255.0 * qc.hsvSaturationF())];
                    ++histo[2][static_cast<int>(255.0 * qc.valueF())];
                }
                else {
                    ++histo[0][static_cast<int>(255.0 * static_cast<float>(qc.hslHue() + 1.0) / 360.0)];
                    ++histo[1][static_cast<int>(255.0 * qc.hslSaturationF())];
                    ++histo[2][static_cast<int>(255.0 * qc.lightnessF())];
                }
                ++total;
            }
        }
    int maxI = 0, cutoff = total / 4;
    for (int j = 0; j < end; ++j)
        for (int i = 1; i < bins - 1; ++i)
            if (histo[j][i] < cutoff)
                maxI = max(maxI, histo[j][i]);
    // Draw histograms.
    maxI = max(maxI, 1);
    double div = static_cast<double>(H / 2 - 1) / static_cast<double>(maxI);
    float fbins = static_cast<float>(bins - 1);
    for (int x = 0; x < bins; ++x) {
        QRgb value = static_cast<QRgb>(bins + x) / 2;
        for  (int j = 0; j < end; ++j) {
            QRgb color = 0xFF000000;
            if (type == RGB && j != 0)
                color += (value << (8 * (2 - (j - 1))));
            else if (type == HSV && j == 0) {
                QColor qc(color);
                qc.setHsvF(static_cast<float>(x) / fbins, 1.0, 1.0);
                color = qc.rgba();
            }
            else if (type == HSL && j == 0) {
                QColor qc(color);
                qc.setHslF(static_cast<float>(x) / fbins, 1.0, 0.5);
                color = qc.rgba();
            }
            else if (type == HSV && j == 1) {
                QColor qc(color);
                qc.setHsvF(0.3f, static_cast<float>(x) / fbins, 1.0f);
                color = qc.rgba();
            }
            else if (type == HSL && j == 1) {
                QColor qc(color);
                qc.setHslF(0.3f, static_cast<float>(x) / fbins, 0.5f);
                color = qc.rgba();
            }
            else if ((type == RGB && j == 0) || (type != RGB && j == 2))
                for (unsigned char i= 0; i < 3; ++i)
                    color += value << (8 * i);
            int rowOffset = (H / 2) * (j / 2);
            int colOffset = x + bins * (j % 2);
            int stop = H / 2 + rowOffset;
            int start = stop - static_cast<int>(static_cast<double>(histo[j][x]) * div);
            start = max(start, rowOffset);
            for (int y = start; y < stop; ++y)
                qi.setPixelColor(colOffset, y, color);
        }
    }
    histograms->setPixmap(QPixmap::fromImage(qi));
    histograms->setFixedSize(histograms->size());
}

QImage MainWindow::kmeans(vector<QColor> centers, int num, eType type) {
    segmented = QImage(processed.size(), QImage::Format_ARGB32_Premultiplied);
    if (num == 0)
        return segmented;
    int w = processed.width(), h = processed.height();
    vector <QColor> clusterCenters(centers);
    int K = static_cast<int>(centers.size());
    vector< vector <unsigned char> > labels(w, vector<unsigned char>(h, 0));
    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(processed.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QColor qc(line[x]);
            double min_dist = colorDistance(clusterCenters[0], qc, type);
            for (int k = 1; k < K; ++k) {
                double distance = colorDistance(clusterCenters[k], qc, type);
                if (distance < min_dist) {
                    min_dist = distance;
                    labels[x][y] = static_cast<unsigned char>(k);
                }
            }
        }
    }
    for (int i = 0; i < num; ++i) {
        for (int k = 0; k < K; ++k) {
            long mean_b = 0, mean_g = 0, mean_r = 0;
            int total = 0;
            int shift;
            if (type == HSV)
                shift = 180 - (clusterCenters[k].hsvHue() + 1);
            else if (type == HSL)
                shift = 180 - (clusterCenters[k].hslHue() + 1);
            for (int y = 0; y < h; ++y) {
                QRgb *line = reinterpret_cast<QRgb *>(processed.scanLine(y));
                for (int x = 0; x < w; ++x) {
                    if (labels[x][y] == k) {
                        QColor qc(line[x]);
                        if (type == RGB) {
                            mean_r += qc.red();
                            mean_g += qc.green();
                            mean_b += qc.blue();
                        }
                        else if (type == HSV) {
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
                            int hue = (qc.hslHue() + 1) + shift;
                            if (hue > 360)
                                hue -= 360;
                            else if (hue < 0)
                                hue += 360;
                            mean_r += hue;
                            mean_g += qc.hslSaturation();
                            mean_b += qc.lightness();
                        }
                        ++total;
                    }
                }
            }
            if (total != 0) {
                mean_r /= total;
                mean_g /= total;
                mean_b /= total;
            }
            if (type == RGB)
                clusterCenters[k] = QColor(static_cast<int>(mean_r), static_cast<int>(mean_g), static_cast<int>(mean_b));
            else if (type == HSV) {
                QColor qc;
                int hue = static_cast<int>(mean_r) - shift;
                if (hue > 360)
                    hue -= 360;
                if (hue < 0)
                    hue += 360;
                qc.setHsv(hue - 1, mean_g, mean_b);
                clusterCenters[k] = qc;
            }
            else if (type == HSL) {
                QColor qc;
                int hue = static_cast<int>(mean_r) - shift;
                if (hue > 360)
                    hue -= 360;
                if (hue < 0)
                    hue += 360;
                qc.setHsl(hue - 1, mean_g, mean_b);
                clusterCenters[k] = qc;
            }
        }
        for (int y = 0; y < h; ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(processed.scanLine(y));
            for (int x = 0; x < w; ++x) {
                QColor qc(line[x]);
                double min_dist = colorDistance(clusterCenters[0], qc, type);
                for (int k = 1; k < K; ++k) {
                    double distance = colorDistance(clusterCenters[k], qc, type);
                    if (distance < min_dist) {
                        min_dist = distance;
                        labels[x][y] = static_cast<unsigned char>(k);
                    }
                }
            }
        }
        ui->statusbar->showMessage(QString(to_string(i).c_str()) + " iterations of " + QString(to_string(num).c_str()) + " completed");
        vector <QRgb> colors;
        for (QColor qc : clusterCenters)
            colors.push_back(qc.rgba());
        for (int y = 0; y < h; ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(segmented.scanLine(y));
            for (int x = 0; x < w; ++x)
                line[x] = colors[labels[x][y]];//segmented.setPixelColor(x, y, clusterCenters[labels[x][y]]);
        }
        toDisplay = segmented.copy();
        repaint();
        QApplication::processEvents();
    }
    vector <QRgb> colors;
    for (QColor qc : clusterCenters)
        colors.push_back(qc.rgba());
    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(segmented.scanLine(y));
        for (int x = 0; x < w; ++x)
            line[x] = colors[labels[x][y]];
    }
    return segmented;
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

vector<float> MainWindow::rgb2lab(QColor qc) {
    // D65/2°
    float xyzRef[3] = {95.047, 100.0, 108.883};
    // rgb to xyz
    float rgb[3] = {qc.redF(), qc.greenF(), qc.blueF()};
    for (unsigned char i = 0; i < 3; ++i) {
        if (rgb[i] > 0.04045f)
            rgb[i] = pow((rgb[i] + 0.055f) / 1.055f, 2.4f);
        else
            rgb[i] /= 12.92f;
        rgb[i] *= 100.0f;
    }
    float xyz[3] = {0.0f};
    xyz[0] = 0.4124 * rgb[0] + 0.3576 * rgb[1] + 0.1805 * rgb[2];
    xyz[1] = 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2];
    xyz[2] = 0.0193 * rgb[0] + 0.1192 * rgb[1] + 0.9505 * rgb[2];
    // xyz to lab
    for (unsigned char i = 0; i < 3; ++i) {
        xyz[i] /= xyzRef[i];
        if (xyz[i] > 0.008856)
            xyz[i] = pow(xyz[i], 1.0 / 3.0);
        else
            xyz[i] = (7.787 * xyz[i]) + 16.0 / 116.0;
    }
    return vector<float>({116.0f * xyz[1] - 16.0f, 500.0f * (xyz[0] - xyz[1]), 200.0f * (xyz[1] - xyz[2])});
}

QColor MainWindow::lab2rgb(vector<float> lab) {
    // D65/2°
    float xyzRef[3] = {95.047f, 100.0f, 108.883f};
    // lab to xyz
    float xyz[3] = {0.0f};
    xyz[1] = (lab[0] + 16.0f) / 116.0f;
    xyz[0] = lab[1] / 500.0f + xyz[1];
    xyz[2] = xyz[1] - lab[2] / 200.0f;
    for (unsigned char i = 0; i < 3; ++i) {
        if (pow(xyz[i], 3.0f) > 0.008856f)
            xyz[i] = pow(xyz[i], 3.0f);
        else
            xyz[i] = (xyz[i] - 16.0f / 116.0f) / 7.787f;
        xyz[i] *= xyzRef[i];
    }
    // xyz to rgb
    float rgb[3] = {0.0f};
    rgb[0] = xyz[0] *  3.2406f + xyz[1] * -1.5372f + xyz[2] * -0.4986f;
    rgb[1] = xyz[0] * -0.9689f + xyz[1] *  1.8758f + xyz[2] *  0.0415f;
    rgb[2] = xyz[0] *  0.0557f + xyz[1] * -0.2040f + xyz[2] *  1.0570f;
    for (unsigned char i = 0; i < 3; ++i) {
        rgb[i] /= 100.0f;
        if (rgb[i] > 0.0031308)
            rgb[i] = 1.055 * pow(rgb[i], 1.0 / 2.4) - 0.055;
        else
            rgb[i] *= 12.92;
    }
    QColor qc;
    qc.setRgbF(rgb[0], rgb[1], rgb[2]);
    return qc;
}

vector <float> MainWindow::getLabScaled(vector <float> lab) {
    vector <float> ret = lab;
    for (unsigned char i = 0; i < 3; ++i)
        ret[i] = scales[i] * (ret[i] - mins[i]);
    return ret;
}



