#include "sensitivitydialog.h"
#include "ui_sensitivitydialog.h"

sensitivityDialog::sensitivityDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sensitivityDialog)
{
    ui->setupUi(this);
    setWindowTitle("Peak Detection");
    lock = 0;
    t1 = t2 = count = 0;
}

sensitivityDialog::~sensitivityDialog() {
    delete ui;
}

void sensitivityDialog::setWork(QImage toProcess, eType t) {
    qi = toProcess;
    type = t;
    process();
    ui->slider->setValue(0);
    ui->spinbox->setValue(0.0);
}

void sensitivityDialog::on_spinbox_valueChanged(double value) {
    val = value;
    ui->slider->setValue(static_cast<int>(10.0 * value));
    process();
}

void sensitivityDialog::on_slider_valueChanged(int value) {
    val = static_cast<double>(value) / 10.0;
    ui->spinbox->setValue(val);
    process();
}

void sensitivityDialog::process() {
    if (lock)
        return;
    ++count;
    lock = 1;
    float extrema = static_cast<double>(val);
    long long time = getTime(0);
    int end = type == RGB ? 4 : 3;
    int H = 3 * bins;
    QImage QI (QSize(bins * 2, H), QImage::Format_ARGB32_Premultiplied);
    QI.fill(0xFF000000);
    vector <QColor> colors;
    vector <vector <float> > histo(4, vector<float>(bins, 0.0f));
    QImage image = qi.copy();
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
    //smooth histograms for peak detection
    for (int j = 0; j < end; ++j)
        for (int i = 0; i < bins; ++i) {
            int center = histo[j][i];
            int left = i == 0 ? histo[j][i] : histo[j][i - 1];
            int right = i == bins - 1 ? histo[j][i] : histo[j][i + 1];
            histo[j][i] = center / 2 + (left + right) / 4;
        }
    vector <vector <int> > peaks(4, vector <int>());
    for (int channel = 0; channel < end; ++channel) {
        vector <float> x0 = histo[channel];
        int minIdx = distance(x0.begin(), min_element(x0.begin(), x0.end()));
        int maxIdx = distance(x0.begin(), max_element(x0.begin(), x0.end()));
        float sel = (x0[maxIdx] - x0[minIdx]) / 4.0;
        for (int i = 0; i < x0.size(); ++i)
            x0[i] *= extrema;
        vector <float> dx(x0.size() - 1);
        for (int i = 1; i < x0.size(); ++i)
            dx[i-1] = x0[i] - x0[i-1];
        replace(dx.begin(), dx.end(), 0.0f, -EPS);
        vector <float> dx0(dx.begin(), dx.end() - 1);
        vector <float> dx0_1(dx.begin() + 1, dx.end());
        vector <float> dx0_2(dx0.size());
        for (int i = 0; i < dx0.size(); ++i)
            dx0_2[i] = dx0[i] * dx0_1[i];
        vector <int> ind;
        // Find where the derivative changes sign
        for (int i = 0; i < dx0_2.size(); ++i)
            if (dx0_2[i] < 0.0)
                ind.push_back(i + 1);
        vector <float> x;
        float leftMin;
        int minMagIdx;
        float minMag;
        for (int i = 0; i < ind.size(); ++i)
            x.push_back(x0[ind[i]]);
        if (x.size() > 2) {
            minMagIdx = distance(x.begin(), min_element(x.begin(), x.end()));
            minMag = x[minMagIdx];
            leftMin = x[0] < x0[0] ? x[0] : x0[0];
        }
        int len = x.size();
        if (len > 2) {
            float tempMag = minMag;
            bool foundPeak = false;
            int ii = x[0] >= x[1] ? 0 : 1;
            float maxPeaks = ceil(static_cast<float>(len) / 2.0);
            vector <int> peakLoc(maxPeaks, 0);
            vector <float> peakMag(maxPeaks, 0.0);
            int cInd = 1;
            int tempLoc;
            while (ii < len) {
                ++ii;
                if (foundPeak) {
                    tempMag = minMag;
                    foundPeak = false;
                }
                if (x[ii - 1] > tempMag && x[ii - 1] > leftMin + sel) {
                    tempLoc = ii - 1;
                    tempMag = x[ii - 1];
                }
                if (ii == len)
                    break;
                ++ii;
                if (!foundPeak && tempMag > sel + x[ii - 1]) {
                    foundPeak = true;
                    leftMin = x[ii - 1];
                    peakLoc[cInd - 1] = tempLoc;
                    peakMag[cInd - 1] = tempMag;
                    ++cInd;
                }
                else if (x[ii - 1] < leftMin)
                    leftMin = x[ii - 1];
            }
            if (!foundPeak) {
                float minAux = x0[x0.size() - 1] < x[x.size() - 1] ? x0[x0.size() - 1] : x[x.size() - 1];
                if (x[x.size() - 1] > tempMag && x[x.size() - 1] > leftMin + sel) {
                    peakLoc[cInd - 1] = len - 1;
                    peakMag[cInd - 1] = x[x.size() - 1];
                    ++cInd;
                }
                else if (!(tempMag >  minAux + sel)) {
                    peakLoc[cInd - 1] = tempLoc;
                    peakMag[cInd - 1] = tempMag;
                    ++cInd;
                }
            }
            if (cInd > 0) {
                vector<int> peakLocTmp(peakLoc.begin(), peakLoc.begin() + cInd - 1);
                peaks[channel].push_back(ind[peakLocTmp[0]]);
                for(int i = 1; i < peakLocTmp.size(); ++i)
                    if (ind[peakLocTmp[i]] != peaks[channel].back() + 1)
                        peaks[channel].push_back(ind[peakLocTmp[i]]);
                if (histo[channel][0] >= 1.5 * histo[channel][1])
                    peaks[channel].insert(peaks[channel].begin(), 0);
                if (histo[channel][bins - 1] >= 1.5 * histo[channel][bins - 2])
                    peaks[channel].push_back(bins - 1);
            }
        }
    }
    int adder = end - 3;
    for (int i = adder; i < end; ++i) {
        sort(peaks[i].begin(), peaks[i].end());
        if (peaks[i].size() >= 2)
            for (int j = 0; j < peaks[i].size() - 1; ++j)
                if (abs(peaks[i][j] - peaks[i][j + 1]) <= 1)
                    peaks[i].erase(peaks[i].begin() + j + 1);
    }
    int Min = static_cast<int>(min(min(peaks[adder].size(), peaks[adder + 1].size()), peaks[adder + 2].size()));
    if (Min != 0) {
        for (int i = adder; i < end; ++i) {
            // midpoint between the closest peaks
            while (peaks[i].size() > Min) {
                int index = -1, diff = INT_MAX;
                for (int j = 0; j < peaks[i].size() - 1; ++j)
                    if (abs(peaks[i][j + 1] - peaks[i][j]) < diff) {
                        diff = abs(peaks[i][j + 1] - peaks[i][j]);
                        index = j;
                    }
                peaks[i][index] = (peaks[i][index] + peaks[i][index]) / 2;
                peaks[i].erase(peaks[i].begin() + index + 1);
            }
            // or remove lowest peak
            for (int k = 0; k < peaks[i].size() - 1; ++k)
                for (int j = k + 1; j < peaks[i].size(); ++j)
                    if (histo[i][peaks[i][k]] > histo[i][peaks[i][j]]) {
                        int temp = peaks[i][k];
                        peaks[i][k] = peaks[i][j];
                        peaks[i][j] = temp;
                    }
        }
    }
    centers.clear();
    if (type == RGB)
        for (int i = 0; i < Min; ++i)
            centers.push_back(QColor(peaks[1][i], peaks[2][i], peaks[3][i]));
    else if (type == HSV)
        for (int i = 0; i < Min; ++i) {
            QColor qc;
            qc.setHsvF(static_cast<float>(peaks[0][i]) / 255.0, static_cast<float>(peaks[1][i]) / 255.0, static_cast<float>(peaks[2][i]) / 255.0);
            centers.push_back(qc);
        }
    else if (type == HSL)
        for (int i = 0; i < Min; ++i) {
            QColor qc;
            qc.setHslF(static_cast<float>(peaks[0][i]) / 255.0, static_cast<float>(peaks[1][i]) / 255.0, static_cast<float>(peaks[2][i]) / 255.0);
            centers.push_back(qc);
        }
    t1 += getTime(time);
    int maxI = 0, cutoff = total / 4;
    for (int j = 0; j < end; ++j)
        for (int i = 1; i < bins - 1; ++i)
            if (histo[j][i] < cutoff)
                maxI = max(maxI, static_cast<int>(histo[j][i]));
    // Draw histograms.
    double div = static_cast<double>(H / 2 - 1) / static_cast<double>(maxI);
    float fbins = static_cast<float>(bins - 1);
    for (int x = 0; x < bins; ++x) {
        QRgb value = static_cast<QRgb>(bins + x) / 2;
        for (int j = 0; j < end; ++j) {
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
            for (int peak : peaks[j])
                if (peak == x) {
                    QColor N(color);
                    N.setRed(255 - N.red());
                    N.setGreen(255 - N.green());
                    N.setBlue(255 - N.blue());
                    color = N.rgba();
                    break;
                }
            for (int y = start; y < stop; ++y)
                QI.setPixelColor(colOffset, y, color);
        }
    }
    QImage proDisp(QI.width() * 2, QI.height() / 2, QImage::Format_ARGB32_Premultiplied);
    QPainter qp(&proDisp);
    qp.drawImage(0, 0, QI.copy(0, 0, QI.width(), QI.height() / 2));
    qp.drawImage(QI.width(), 0, QI.copy(0, QI.height() / 2, QI.width(), QI.height() / 2));
    qp.end();
    ui->label->setPixmap(QPixmap::fromImage(proDisp));
    t2 += getTime(time);
    lock = 0;
}

void sensitivityDialog::on_buttonBox_accepted() {
    cout << (t1 / count) << "ms peak detection avg" << endl;
    cout << (t2 / count) << "ms peak detection and histogram drawing avg" << endl;
    done(1);
}

void sensitivityDialog::on_buttonBox_rejected() {
    done(0);
}

vector<QColor> sensitivityDialog::getPoints() {
    return centers;
}


