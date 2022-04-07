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

const int threadCnt = QThread::idealThreadCount();
const iMat sobel = {{-1, -2, -1},
                    {0,   0,  0},
                    {1,   2,  1}};
const QString views[] = {"og", "ogEdge", "processedEdge", "processed"};
const vector <string> acceptedImportImageFormats = {"bmp", "jpg", "jpeg", "png", "ppm", "xbm", "xpm", "gif", "pbm", "pgm"};

const double pi = 3.14159265359;
const float EPS = 2.2204e-16f;
const int bins = 256;

const int hueDiff = 30;
const int satDiffMult = 2;
const int litDiffMult = 1;

const double scale = 0.176776695;

const QStringList eTypes {"RGB", "HSV / HSB", "HSL / HSI", "CIE LAB"};
enum eType {RGB, HSV, HSL, LAB};

const float mins[3] = {0.0, -86.1847, -107.864};
const float scales[3] = {2.55, 1.3825, 1.2602};

static long long getTime(long long initial = 0) {
    return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count() - initial;
}


#endif // PREDEFS_H
