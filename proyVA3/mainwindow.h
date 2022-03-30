

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

    Ptr<ORB> orbDetector;
    Ptr<BFMatcher> matcher;

    Mat copyWindow();

public slots:
    void compute();
    void start_stop_capture(bool start);
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow(QPointF p);
    void addObject();
    void deleteObject();
    void showImage(int index);
    void collectionMatching();
};


#endif // MAINWINDOW_H
