#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/dnn/dnn.hpp>

#include <QtWidgets/QFileDialog>
#include <iostream>
#include <fstream>
#include <ui_lFilterForm.h>
#include <ui_combineForm.h>

#include <QListWidgetItem>
#include <QCheckBox>

#include <imgviewer.h>

using namespace cv;

namespace Ui {
    class MainWindow;
}

class LFilterDialog : public QDialog, public Ui::LFilterForm
{
    Q_OBJECT

    public:
    LFilterDialog(QDialog *parent=0) : QDialog(parent){
        setupUi(this);
    }
};

class CombineDialog : public QDialog, public Ui::CombineForm
{
    Q_OBJECT

    public:
    CombineDialog(QDialog *parent=0) : QDialog(parent){
        setupUi(this);
    }
};

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
    ImgViewer *visorS, *visorD, *visorC;
    Mat colorImage, grayImage;
    Mat destColorImage, destGrayImage;
    bool winSelected;
    Rect imageWindow;
    LFilterDialog lFilterDialog;
    CombineDialog combineDialog;
    Mat backgroundImage, combinedImage;

    dnn::Net net;
    std::vector<Vec3b> colorTable;
    std::vector<bool> availableCategories;

    void fillColorTable();
    Size adjustSize(Mat fileImage);
    Mat processOutput(Mat output, int height, int width);
    Mat mixImages(Mat input);

public slots:
    void compute();
    void start_stop_capture(bool start);
    void change_color_gray(bool color);
    void loadImage();
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow(QPointF p);
    void segmentImage();
    void readCategories();
    void updateCategories();
    void saveImage();
    void loadBackground();
    void pasteImage();
    void copyObject(QPointF position);
};


#endif // MAINWINDOW_H
