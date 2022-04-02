#ifndef PREDEFS_H
#define PREDEFS_H

#include <vector>
#include <string>
#include <QStringList>
#include <time.h>
#include <chrono>

using std::vector;
using std::string;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

const int threadCnt = QThread::idealThreadCount();

const QString views[] = {"og", "ogEdge", "processedEdge", "processed"};
const vector <string> acceptedImportImageFormats = {"bmp", "jpg", "jpeg", "png", "ppm", "xbm", "xpm", "gif", "pbm", "pgm"};

const double pi = 3.14159265359;
const int bins = 256;

const int hueDiff = 30;
const int satDiffMult = 2;
const int litDiffMult = 1;

const double scale = 0.176776695;

const QStringList eTypes {"RGB", "HSV / HSB", "HSL / HSI"};
enum eType {RGB, HSV, HSL};

static long long getTime(long long initial = 0) {
    return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count() - initial;
}

#endif // PREDEFS_H
