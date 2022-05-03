#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cap = new VideoCapture(0);
    if(!cap->isOpened())
        cap = new VideoCapture(1);
    winSelected = false;

    colorImage.create(240,320,CV_8UC3);
    grayImage.create(240,320,CV_8UC1);
    destColorImage.create(240,320,CV_8UC3);
    destGrayImage.create(240,320,CV_8UC1);

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);

    segmentedImage.create(240,320,CV_32SC1);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(mouseSelection(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(mouseClic(QPointF)),this,SLOT(deselectWindow(QPointF)));
    connect(ui->loadButton,SIGNAL(clicked()),this,SLOT(loadImageFromFile()));
    timer.start(60);


}

MainWindow::~MainWindow()
{
    delete ui;
    delete cap;
    delete visorS;
    delete visorD;
    grayImage.release();
    colorImage.release();
    destGrayImage.release();
    destColorImage.release();
    segmentedImage.release();

}

void MainWindow::compute()
{

    if(ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> colorImage;

        cv::resize(colorImage, colorImage, Size(320, 240));

        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);

    }

    regionGrowing(grayImage);

    colorSegmentedImage();

    if(winSelected)
    {
        visorS->drawSquare(QPointF(imageWindow.x+imageWindow.width/2, imageWindow.y+imageWindow.height/2), imageWindow.width,imageWindow.height, Qt::green );
    }
    visorS->update();
    visorD->update();

}

void MainWindow::regionGrowing(Mat image)
{
    int regId=0;
    RegSt newReg;
    Mat maskImage, edges;
    int thresh = 10;


    Canny(image, edges, 40, 120);

    segmentedImage.setTo(-1);
    regionsList.clear();

    copyMakeBorder(edges, maskImage, 1, 1, 1, 1, BORDER_CONSTANT, Scalar(1));

    for(int y=0; y<240; y++)
        for(int x=0; x<320; x++)
        {

            if(segmentedImage.at<int>(y,x)==-1 && edges.at<uchar>(y,x)==0)
            {
                newReg.gray=image.at<uchar>(y,x);

                Rect winReg;

                floodFill(image, maskImage, Point(x,y),Scalar(1),&winReg, thresh,thresh,  FLOODFILL_MASK_ONLY| 4 | ( 1 << 8 ) );

                newReg.npoints=copyRegion(image, maskImage, regId, winReg, newReg.gray);
                regionsList.push_back(newReg);
                regId++;

            }
        }

    int winner=-1;
    int minDiff=300, diff;
    for(int y=0; y<240; y++)
        for(int x=0; x<320; x++)
        {
            if(segmentedImage.at<int>(y,x)==-1)
            {
                winner=-1;
                minDiff=300;
                for(int vy=-1; vy<=1; vy++)
                    for(int vx=-1; vx<=1; vx++)
                    {
                        if(vx!=0 || vy!=0)
                        {
                            if((y+vy)>=0 && (y+vy)<240 && (x+vx)>=0 && (x+vx)<320 && segmentedImage.at<int>(y+vy,x+vx)!=-1 && edges.at<uchar>(y+vy,x+vx)==0)
                            {
                                regId=segmentedImage.at<int>(y+vy,x+vx);
                                diff=abs(regionsList[regId].gray - image.at<uchar>(y,x));
                                if(diff<minDiff)
                                {
                                    minDiff=diff;
                                    winner=regId;
                                }
                            }

                        }
                    }
                if(winner!=-1)
                    segmentedImage.at<int>(y,x)=winner;

            }

        }
}

int MainWindow::copyRegion(Mat image, Mat maskImage, int id, Rect r, uchar & mgray)
{
    int meanGray = 0;
    int nPoints = 0;

    for(int y=r.y; y<r.y+r.height; y++)
        for(int x=r.x; x<r.x+r.width; x++)
        {
            if(y>=0 && x>=0 && y<240 && x<320)
            {
                if(segmentedImage.at<int>(y,x)==-1 && maskImage.at<uchar>(y+1,x+1)==1)
                {
                    segmentedImage.at<int>(y,x) = id;
                    meanGray += image.at<uchar>(y,x);
                    nPoints++;
                }
            }
        }

    mgray = meanGray/nPoints;
    return nPoints;

}


void MainWindow::colorSegmentedImage()
{
    int id;

    for(int y=0; y<240; y++)
        for(int x=0; x<320; x++)
        {
           id = segmentedImage.at<int>(y,x);
           destGrayImage.at<uchar>(y,x) = regionsList[id].gray;

        }
}


void MainWindow::start_stop_capture(bool start)
{
    if(start)
        ui->captureButton->setText("Stop capture");
    else
        ui->captureButton->setText("Start capture");
}

void MainWindow::change_color_gray(bool color)
{
    if(color)
    {
        ui->colorButton->setText("Gray image");
        visorS->setImage(&colorImage);
        visorD->setImage(&destColorImage);

    }
    else
    {
        ui->colorButton->setText("Color image");
        visorS->setImage(&grayImage);
        visorD->setImage(&destGrayImage);

    }
}

void MainWindow::selectWindow(QPointF p, int w, int h)
{
    QPointF pEnd;
    if(w>0 && h>0)
    {
        imageWindow.x = p.x()-w/2;
        if(imageWindow.x<0)
            imageWindow.x = 0;
        imageWindow.y = p.y()-h/2;
        if(imageWindow.y<0)
            imageWindow.y = 0;
        pEnd.setX(p.x()+w/2);
        if(pEnd.x()>=320)
            pEnd.setX(319);
        pEnd.setY(p.y()+h/2);
        if(pEnd.y()>=240)
            pEnd.setY(239);
        imageWindow.width = pEnd.x()-imageWindow.x;
        imageWindow.height = pEnd.y()-imageWindow.y;

        winSelected = true;
    }
}

void MainWindow::deselectWindow(QPointF p)
{
    std::ignore = p;
    winSelected = false;

}

void MainWindow::loadImageFromFile()
{
    ui->captureButton->setChecked(false);
    ui->captureButton->setText("Start capture");
    disconnect(&timer,SIGNAL(timeout()),this,SLOT(compute()));

    QString fileName = QFileDialog::getOpenFileName(this, tr("Load image from file"),".", tr("Images (*.png *.xpm *.jpg)"));
    if(!fileName.isNull())
    {
        Mat imfromfile = imread(fileName.toStdString(), IMREAD_COLOR);
        Size imSize = imfromfile.size();
        if(imSize.width!=320 || imSize.height!=240)
            cv::resize(imfromfile, imfromfile, Size(320, 240));

        if(imfromfile.channels()==1)
        {
            imfromfile.copyTo(grayImage);
            cvtColor(grayImage,colorImage, COLOR_GRAY2RGB);
        }

        if(imfromfile.channels()==3)
        {
            imfromfile.copyTo(colorImage);
            cvtColor(colorImage, colorImage, COLOR_BGR2RGB);
            cvtColor(colorImage, grayImage, COLOR_RGB2GRAY);
        }

    }
    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));


}


