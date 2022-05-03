#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <imgviewer.h>

#define MAXNREGS 1000


using namespace cv;
using namespace std;

typedef struct{
    uchar gray;
    int npoints;
}RegSt;

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
    ImgViewer *visorS, * visorD;
    Mat colorImage, grayImage, destColorImage, destGrayImage;
    Mat segmentedImage;
    bool winSelected;
    Rect imageWindow;
    vector<RegSt> regionsList;

    void regionGrowing(Mat image);
    int copyRegion(Mat image, Mat maskImage, int id, Rect r, uchar & mgray);
    void colorSegmentedImage();

public slots:
    void compute();
    void start_stop_capture(bool start);
    void change_color_gray(bool color);
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow(QPointF p);
    void loadImageFromFile();

};


#endif // MAINWINDOW_H
