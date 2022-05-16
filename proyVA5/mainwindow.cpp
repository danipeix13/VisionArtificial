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
    dispImage.create(240,320,CV_32FC1);
    dispGray.create(240,320,CV_8UC1);
    dispCheckImage.create(240,320,CV_8UC1);
    fixed.create(240,320,CV_8UC1);
    fixed.setTo(0);
    dispImage.setTo(0);

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);
    visorDisp = new ImgViewer(&dispGray, ui->dispFrm);
    visorTrueDisp = new ImgViewer(&dispCheckImage, ui->dispCheckFrm);

    segmentedImage.create(240,320,CV_32SC1);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));

    // UI
    //connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    //connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(mouseSelection(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(mouseClic(QPointF)),this,SLOT(deselectWindow(QPointF)));
    connect(ui->loadButton,SIGNAL(clicked()),this,SLOT(loadImageFromFile()));
    connect(ui->dispInitBtn,SIGNAL(clicked()),this,SLOT(obtainCorners()));
    connect(ui->loadGTBtn,SIGNAL(clicked()),this,SLOT(loadTrueDispImage()));
    connect(visorDisp,SIGNAL(mouseClic(QPointF)),this,SLOT(getPixelValues(QPointF)));

    timer.start(60);


}

MainWindow::~MainWindow()
{
    delete ui;
    delete cap;
    delete visorS;
    delete visorD;
    delete visorDisp;
    delete visorTrueDisp;
    grayImage.release();
    colorImage.release();
    destGrayImage.release();
    destColorImage.release();
    segmentedImage.release();
    dispImage.release();
    dispCheckImage.release();

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
//    obtainCorners();
    //regionGrowing(grayImage);

    //colorSegmentedImage();

    if(ui->kpChbx->isChecked())
    {
        for (Point p : leftImageCorners)
            visorS->drawSquare(QPointF(p.x, p.y), 2, 2, Qt::red);
        for (Point p : rightImageCorners)
            visorD->drawSquare(QPointF(p.x, p.y), 2, 2, Qt::red);

        for (Vec4f c : correspondencies)
        {
            visorS->drawSquare(QPointF(c[0], c[1]), 2, 2, Qt::green);
            visorD->drawSquare(QPointF(c[2], c[3]), 2, 2, Qt::green);
        }

    }

    if(winSelected)
    {
        visorS->drawSquare(QPointF(imageWindow.x+imageWindow.width/2, imageWindow.y+imageWindow.height/2), imageWindow.width,imageWindow.height, Qt::green );
    }
    visorS->update();
    visorD->update();
    visorDisp->update();
    visorTrueDisp->update();

}

void MainWindow::obtainCorners()
{
    qDebug() << "obtainCorners";

    leftImageCorners.clear();
    rightImageCorners.clear();
    correspondencies.clear();

    goodFeaturesToTrack(grayImage, leftImageCorners, 0, 0.01, 5);
    goodFeaturesToTrack(destGrayImage, rightImageCorners, 0, 0.01, 5);

    for(int i = 0 ; i < leftImageCorners.size() ; i++)
    {
        float bestResult = -5.0;
        Point2f bestMatch, left = leftImageCorners[i];
        Rect leftRect = getRect(left);


        if(leftRect.x >= 0 && leftRect.y > 0 && leftRect.x + leftRect.width < 320 && leftRect.y + leftRect.height < 240)
        {
            Mat leftImageWindow = Mat(grayImage, leftRect), result;
            for(int j = 0; j < rightImageCorners.size(); j++)
            {

                Point2f right =  rightImageCorners[j];

                if((left.y == right.y || left.y == right.y-1 || left.y == right.y+1) && left.x >= right.x)
                {
                    Rect rightRect = getRect(right);
                    if(rightRect.x >= 0 && rightRect.y > 0 && rightRect.x + rightRect.width < 320 && rightRect.y + rightRect.height < 240)
                    {
                        Mat rightImageWindow = Mat(destGrayImage, rightRect);
                        cv::matchTemplate(leftImageWindow, rightImageWindow, result, TM_CCOEFF_NORMED);

                        if (result.at<float>(Point(0,0)) > bestResult)
                        {
                            bestMatch = right;
                            bestResult = result.at<float>(Point(0,0));
                        }
                    }
                    //else qDebug()<< __FUNCTION__ << "  VAYA :') en Y";
                }
            }
        }
        //else qDebug()<< __FUNCTION__ << "VAYA :') en X";


        if (bestResult >= 0.8)
        {
            fixed.at<uchar>(left.y, left.x) = 1;
            dispImage.at<float>(left.y, left.x) = left.x - bestMatch.x;
            correspondencies.push_back(Vec4i{left.x, left.y, bestMatch.x, bestMatch.y});
        }
    }

    regionGrowing(grayImage);

    for (RegSt &reg: regionsList)
    {
        reg.nFijos = 0;
        reg.dMedia = 0;
    }

    int id, nPoints, grayLevel;

    for(int y = 0; y < dispImage.rows; y++)
    {
        for(int x = 0; x < dispImage.cols; x++)
        {
            if(fixed.at<uchar>(y, x) == 1)
            {
                id = segmentedImage.at<int>(y, x);
                regionsList[id].nFijos++;
                regionsList[id].dMedia += dispImage.at<float>(y, x);
            }
        }
    }

    for (RegSt &reg: regionsList)
    {
        if(reg.nFijos == 0)
        {
            reg.dMedia = 0;
        }
        else
        {
            reg.dMedia = reg.dMedia / reg.nFijos;
        }
    }

    for(int y = 0; y < dispGray.rows; y++)
    {
        for(int x = 0; x < dispGray.cols; x++)
        {
            if(fixed.at<uchar>(y, x) == 0)
            {
                id = segmentedImage.at<int>(y, x);
                dispGray.at<uchar>(y, x) = regionsList[id].dMedia;
            }
            else
                dispGray.at<uchar>(y, x) = dispImage.at<float>(y, x);

            dispGray.at<uchar>(y, x) = dispGray.at<uchar>(y, x) * 3 * 240 / 320;
            std::cout << dispGray.at<uchar>(y, x) << std::endl;
        }
    }

    qDebug() << "KLUFDGOAH";

//    segmentedImage.copyTo(dispImage);
//      colorSegmentedImage();


}

Rect MainWindow::getRect(Point2f src)
{
    int margin = 6;
    Rect r = Rect(Point(src.x-margin, src.y-margin), Point(src.x+margin, src.y+margin));
    return r;
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

                newReg.nPoints=copyRegion(image, maskImage, regId, winReg, newReg.gray);
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
           qDebug() << "ColorSegmentedImage";
           id = segmentedImage.at<int>(y,x);
           dispImage.at<uchar>(y,x) = regionsList[id].gray;
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
        visorD->setImage(&destGrayImage);connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));

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

    // view 1 && view5??

    ui->captureButton->setChecked(false);
    ui->captureButton->setText("Start capture");
    disconnect(&timer,SIGNAL(timeout()),this,SLOT(compute()));

    QStringList nameList = QFileDialog::getOpenFileNames(this, tr("Load image from file"),".", tr("Images (*.png *.xpm *.jpg)"));

    if(nameList.size() == 2)
    {
        bool dst = true;
        for (QString name: nameList)
        {
            Mat imfromfile = imread(name.toStdString(), IMREAD_COLOR);
            Size imSize = imfromfile.size();
            if(imSize.width!=320 || imSize.height!=240)
                cv::resize(imfromfile, imfromfile, Size(320, 240));

            Mat dest;
            if (dst)
                dest = grayImage;
            else
                dest = destGrayImage;

            if(imfromfile.channels()==1)
            {
                imfromfile.copyTo(grayImage);
                cvtColor(grayImage,dest, COLOR_GRAY2RGB);
            }

            if(imfromfile.channels()==3)
            {
                imfromfile.copyTo(colorImage);
                cvtColor(colorImage, colorImage, COLOR_BGR2RGB);
                cvtColor(colorImage, dest, COLOR_RGB2GRAY);
            }
            dst = !dst;
        }
    }
    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));


}

void MainWindow::loadTrueDispImage()
{
    ui->captureButton->setChecked(false);
    ui->captureButton->setText("Start capture");
    disconnect(&timer,SIGNAL(timeout()),this,SLOT(compute()));

    QString name = QFileDialog::getOpenFileName(this, tr("Load image from file"),".", tr("Images (*.png *.xpm *.jpg)"));

    Mat imfromfile = imread(name.toStdString(), IMREAD_COLOR);
    Size imSize = imfromfile.size();
    if(imSize.width!=320 || imSize.height!=240)
        cv::resize(imfromfile, imfromfile, Size(320, 240));

    imfromfile.copyTo(dispCheckImage);
    cvtColor(dispCheckImage,dispCheckImage, COLOR_BGR2GRAY);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
}

void MainWindow::getPixelValues(QPointF point)
{
    float x = point.x(), y = point.y(), T = 160/*mm*/, f = 3740 /*pixeles*/, addValue = 300;

    float disp = dispGray.at<uchar>(point.y(), point.x());
    disp = 103;
    float newDisp = disp + addValue;
    ui->estimatedLCD->display(disp);
    ui->xEst->display((-x)*T/newDisp);
    ui->yEst->display((-y)*T/newDisp);
    ui->zEst->display(f*T/newDisp);
    std::cout << "NUESTRA DISP:" << disp << endl;


    float trueDisp = dispCheckImage.at<uchar>(point.y(), point.x());
    std::cout << "DISP REAL:" << trueDisp << endl;
    newDisp = trueDisp + addValue;
    ui->trueLCD->display(trueDisp);
    ui->xTrue->display((-x)*T/newDisp);
    ui->yTrue->display((-y)*T/newDisp);
    ui->zTrue->display(f*T/newDisp);
}

