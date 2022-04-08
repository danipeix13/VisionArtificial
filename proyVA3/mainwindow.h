

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <imgviewer.h>
#include <opencv2/features2d.hpp>
#include <iostream>
#include <vector>
#include <array>
#include <ranges>
#include <iterator>

using namespace cv;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QTimer timer;

    VideoCapture *cap;
    ImgViewer *visorS, *visorD;
    Mat colorImage, grayImage;
    Mat destColorImage, destGrayImage;
    bool winSelected;
    Rect imageWindow;
    std::vector<Mat> images;
    std::vector<int> collect2object;

    std::vector<std::vector<std::vector<KeyPoint>>> objectKP; //Acceso: objectKP[objeto][escala] --> std::vector<KeyPoint> (keypoints)
    std::vector<std::vector<Mat>> objectDesc; //Acceso: objectDesc[objeto][escala] --> Mat(descriptor)
    std::vector<float> scaleFactors = {0.75, 1.0, 1.25};
    const int N_SCALES = scaleFactors.size(), N_OBJECTS = 8;

    Ptr<ORB> orbDetector;
    Ptr<BFMatcher> matcher;
    struct Match {  std::vector<DMatch> value; int i, j; };
    std::vector<QColor> colors = {Qt::white,     Qt::red,       Qt::green,      Qt::blue,
                                  Qt::gray,      Qt::cyan,      Qt::magenta,    Qt::yellow,
                                  Qt::black,     Qt::darkRed,   Qt::darkBlue,   Qt::darkGreen,
                                  Qt::darkGray,  Qt::darkCyan,  Qt::darkMagenta,Qt::darkYellow};
    Mat temporaryMat;

    Mat copyWindow();
    void collectionMatching();
    std::vector<std::vector<std::vector<DMatch>>> orderMatches(std::vector<std::vector<DMatch>> matches);
    void bestMatch(std::vector<std::vector<std::vector<DMatch>>> ordered_matches, std::vector<Match> &bestMatches);
    void pointsCorrespondence(std::vector<DMatch> ordered_matches, std::vector<KeyPoint> imageKp, int bestObject,int bestScale,
                              std::vector<Point2f> &imagePoints, std::vector<Point2f> &objectPoints);
    bool getAndApplyHomography(std::vector<Point2f> imagePoints, std::vector<Point2f> objectPoints, int bestObject, int bestScale, std::vector<Point2f> &imageCorners);
    void paintResult(std::vector<Point2f> imageCorners, QString name, QColor color);

public slots:
    void compute();
    void start_stop_capture(bool start);
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow(QPointF p);
    void addObject();
    void deleteObject();
    void saveCollection();
    void loadCollection();
    void showImage(int index);
};


#endif // MAINWINDOW_H
