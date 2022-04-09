#ifndef PREDEFS_H
#define PREDEFS_H

#include <QThread>
#include <QColor>
#include <vector>
#include <string>
#include <QStringList>
#include <time.h>
#include <chrono>
#include <iostream>
#include <cmath>
#include <string>
#include <list>
#include <ios>

using Qt::Key;
using std::cout;
using std::endl;
using std::max;
using std::min;
using std::string;
using std::to_string;
using std::list;
using std::sort;
using std::hex;

using std::vector;
using std::string;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

#define iMat vector< vector <int> >
#define fMat vector< vector <float> >
#define cMat vector< vector <unsigned char> >
#define toi static_cast<int>
#define tof static_cast<float>
#define tod static_cast<double>
#define toc static_cast<unsigned char>

const int threadCnt = QThread::idealThreadCount();
const iMat sobel = {{-1, -2, -1},
                    {0,   0,  0},
                    {1,   2,  1}};
const QString views[] = {"og", "ogEdge", "processedEdge", "processed", "segmented"};
const vector <string> acceptedImportImageFormats = {"bmp", "jpg", "jpeg", "png", "ppm", "xbm", "xpm", "gif", "pbm", "pgm"};

const double pi = 3.14159265359;
const float EPS = 2.2204e-16f;
const int bins = 256;

const int hueDiff = 30;
const int satDiffMult = 2;
const int litDiffMult = 1;

const double scale = 0.176776695;

const QStringList eTypes {"RGB", "HSV / HSB", "HSL / HSI", "CIE L*A*B*"};
enum eType {RGB, HSV, HSL, LAB};

const float mins[3] = {0.0f, -86.1847f, -107.864f};
const float scales[3] = {2.55f, 1.3825f, 1.2602f};

static long long getTime(long long initial = 0) {
    return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count() - initial;
}

static vector <float> rgb2lab(QColor qc) {
    // D65/2°
    float xyzRef[3] = {95.047f, 100.0f, 108.883f};
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
    xyz[0] = 0.4124f * rgb[0] + 0.3576f * rgb[1] + 0.1805f * rgb[2];
    xyz[1] = 0.2126f * rgb[0] + 0.7152f * rgb[1] + 0.0722f * rgb[2];
    xyz[2] = 0.0193f * rgb[0] + 0.1192f * rgb[1] + 0.9505f * rgb[2];
    // xyz to lab
    for (unsigned char i = 0; i < 3; ++i) {
        xyz[i] /= xyzRef[i];
        if (xyz[i] > 0.008856f)
            xyz[i] = pow(xyz[i], 1.0f / 3.0f);
        else
            xyz[i] = (7.787f * xyz[i]) + 16.0f / 116.0f;
    }
    return vector<float>({116.0f * xyz[1] - 16.0f, 500.0f * (xyz[0] - xyz[1]), 200.0f * (xyz[1] - xyz[2])});
}

static QColor lab2rgb(vector<float> lab) {
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
        if (rgb[i] > 0.0031308f)
            rgb[i] = 1.055f * pow(rgb[i], 1.0f / 2.4f) - 0.055f;
        else
            rgb[i] *= 12.92f;
    }
    QColor qc;
    qc.setRgbF(rgb[0], rgb[1], rgb[2]);
    return qc;
}

static vector <float> getLabScaled(vector <float> lab) {
    vector <float> ret = lab;
    for (unsigned char i = 0; i < 3; ++i)
        ret[i] = scales[i] * (ret[i] - mins[i]);
    return ret;
}

static vector <float> getLabDescaled(int l, int a, int b) {
    vector <float> ret({tof(l), tof(a), tof(b)});
    for (unsigned char i = 0; i < 3; ++i)
        ret[i] = ret[i] / scales[i] + mins[i];
    return ret;
}



#endif // PREDEFS_H
