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
    sobel.push_back({-1, -2, -1});
    sobel.push_back({ 0,  0,  0});
    sobel.push_back({ 1,  2,  1});
}

MainWindow::~MainWindow() {
    delete ui;
    delete histograms;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    Key key = Key(event->key());
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
    else if (key == Qt::Key_Alt) {
        processed.save(filename + "_processed_" + to_string(passes).c_str() + ".png");
    }
    else if (key == Qt::Key_Space)
        compute();
    else if (key == Qt::Key_Control) {
        bool ok = false;
        int ret = QInputDialog::getInt(this, "8810", "Process cycles to complete", 0, 0, 20, 1, &ok);
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
        processed = og;
        w = og.width();
        h = og.height();
        edL_p = vector< vector<int> > (w, vector<int>(h, 0));
        edL = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
        compute();
        toDisplay = og;
    }
    ui->statusbar->showMessage(QString(to_string(passes).c_str()) + " cycles completed");
    repaint();
}

void MainWindow::paintEvent(QPaintEvent *event) {
    if (toDisplay.isNull())
        return;
    QPainter qp(this);
    qp.drawImage(0, 0, toDisplay.scaledToWidth(1.75 * toDisplay.width(), Qt::SmoothTransformation));
}

vector< vector<float> > MainWindow::getBoxBlur(int width) {
    float w = static_cast<float>(width * width);
    vector<float> sub(width, 1.0 / w);
    return vector< vector<float> > (width, sub);
}

vector< vector<float> > MainWindow::getConeBlur(int width) {
    float r = static_cast<float>(width) / 2.0;
    vector< vector<float> > vec(width, vector<float>(width, 0.0));
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
    int kWidth = 1 + 2 * (passes / 2), offset = kWidth / 2;
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
    vector< vector<float> > blur = getConeBlur(kWidth);
    cout << "create cone blur took " << getTime(time) << "ms" << endl;
    time = getTime(0);
    QImage work = processed;
    int diff = (3 * passes) / 2;
    for (int j = 0; j < h; ++j) {
        QRgb *line = reinterpret_cast<QRgb *>(processed.scanLine(j));
        QRgb *line2 = reinterpret_cast<QRgb *>(work.scanLine(j));
        for (int i = 0; i < w; ++i) {
            float rt = 0.0, gt = 0.0, bt = 0.0;
            QColor qc(line[i]), center = qc;
            int huePro = qc.hslHue() + 1;
            int satPro = qc.hslSaturation();
            int valPro = qc.lightness();
            for (int n = 0; n < kWidth; ++n) {
                QRgb *line3 = nullptr;
                int J = (n - offset) + j;
                if (J >= 0 && J < h)
                    line3 = reinterpret_cast<QRgb *>(work.scanLine((n - offset) + j));
                for (int m = 0; m < kWidth; ++m) {
                    if ((m - offset) + i < 0 || (m - offset) + i >= w || line3 == nullptr)
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
            QColor dL = edL.pixelColor(i, j);
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
    cout << "blurring took " << getTime(time) << "ms" << endl;
    processed = work;
    ++passes;
    toDisplay = processed.copy();
    repaint();
    QApplication::processEvents();
    cout << "process complete. passes " << passes << endl;
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
                    ++histo[0][qc.hsvHue()];
                    ++histo[1][qc.hsvSaturation()];
                    ++histo[2][qc.value()];
                }
                else {
                    ++histo[0][qc.hslHue()];
                    ++histo[1][qc.hslSaturation()];
                    ++histo[2][qc.lightness()];
                }
                ++total;
            }
        }
    int maxI = 0, cutoff = total / 4;
    for (int j = 0; j < 4; ++j)
        for (int i = 1; i < bins - 1; ++i)
            if (histo[j][i] < cutoff)
                maxI = max(maxI, histo[j][i]);
    // Draw histograms.
    double div = static_cast<double>(H / 2 - 1) / static_cast<double>(maxI);
    float fbins = static_cast<float>(bins - 1);
    for (int x = 0; x < bins; ++x) {
        QRgb value = static_cast<QRgb>(bins + x) / 2;
        for  (int j = 0; j < 4; ++j) {
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
                qc.setHsvF(0.3, static_cast<float>(x) / fbins, 1.0);
                color = qc.rgba();
            }
            else if (type == HSL && j == 1) {
                QColor qc(color);
                qc.setHslF(0.3, static_cast<float>(x) / fbins, 0.5);
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
