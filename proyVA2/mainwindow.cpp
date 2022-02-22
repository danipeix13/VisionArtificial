#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cap = new VideoCapture(0);
    winSelected = false;

    colorImage.create(240,320,CV_8UC3);
    grayImage.create(240,320,CV_8UC1);
    destColorImage.create(240,320,CV_8UC3);
    destColorImage.setTo(0);
    destGrayImage.create(240,320,CV_8UC1);
    destGrayImage.setTo(0);

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);

    visorHistoS = new ImgViewer(260,150, (QImage *) NULL, ui->histoFrameS);
    visorHistoD = new ImgViewer(260,150, (QImage *) NULL, ui->histoFrameD);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(mouseSelection(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(mouseClic(QPointF)),this,SLOT(deselectWindow(QPointF)));

    connect(ui->pixelTButton,SIGNAL(clicked()),&pixelTDialog,SLOT(show()));
    connect(pixelTDialog.okButton,SIGNAL(clicked()),&pixelTDialog,SLOT(hide()));

    connect(ui->kernelButton,SIGNAL(clicked()),&lFilterDialog,SLOT(show()));
    connect(lFilterDialog.okButton,SIGNAL(clicked()),&lFilterDialog,SLOT(hide()));

    connect(ui->operOrderButton,SIGNAL(clicked()),&operOrderDialog,SLOT(show()));
    connect(operOrderDialog.okButton,SIGNAL(clicked()),&operOrderDialog,SLOT(hide()));

    connect(ui->loadButton,SIGNAL(clicked()),this,SLOT(loadImage()));
    connect(ui->saveButton,SIGNAL(clicked()),this,SLOT(saveImage()));

    timer.start(30);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete cap;
    delete visorS;
    delete visorD;
    colorImage.release();
    grayImage.release();
    destColorImage.release();
    destGrayImage.release();
}

void MainWindow::compute()
{

    //Captura de imagen
    if(ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> colorImage;
        cv::resize(colorImage, colorImage, Size(320,240));
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);

    }


    //En este punto se debe incluir el código asociado con el procesamiento de cada captura


    //Actualización de los visores
     if(!ui->colorButton->isChecked())
     {
         updateHistograms(grayImage, visorHistoS);
         updateHistograms(destGrayImage, visorHistoD);
     }
     else
     {
         Mat aux_image, channels[3];

         cv::cvtColor(colorImage, aux_image, cv::COLOR_RGB2YUV);
         split(aux_image, channels);
         updateHistograms(channels[0], visorHistoS);

         cv::cvtColor(destColorImage, aux_image, cv::COLOR_RGB2YUV);
         split(aux_image, channels);
         updateHistograms(channels[0], visorHistoD);
     }

     if(winSelected)
     {
         visorS->drawSquare(QPointF(imageWindow.x+imageWindow.width/2, imageWindow.y+imageWindow.height/2), imageWindow.width,imageWindow.height, Qt::green );
     }
     visorS->update();
     visorD->update();
     visorHistoS->update();
     visorHistoD->update();

 }

 void MainWindow::updateHistograms(Mat image, ImgViewer * visor)
 {
     if(image.type() != CV_8UC1) return;

     Mat histogram;
     int channels[] = {0,0};
     int histoSize = 256;
     float grange[] = {0, 256};
     const float * ranges[] = {grange};
     double minH, maxH;

     calcHist( &image, 1, channels, Mat(), histogram, 1, &histoSize, ranges, true, false );
     minMaxLoc(histogram, &minH, &maxH);

     float maxY = visor->getHeight();

     for(int i = 0; i<256; i++)
     {
         float hVal = histogram.at<float>(i);
         float minY = maxY-hVal*maxY/maxH;

         visor->drawLine(QLineF(i+2, minY, i+2, maxY), Qt::red);
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
        imageWindow.width = pEnd.x()-imageWindow.x+1;
        imageWindow.height = pEnd.y()-imageWindow.y+1;

        winSelected = true;
    }
}

void MainWindow::deselectWindow(QPointF p)
{
    std::ignore = p;
    winSelected = false;
}

void MainWindow::loadImage()
{
    timer.stop();
    auto fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "/home/alumno/Imágenes", tr("Image Files (*.png *.jpg *.bmp)"));
    timer.start(30);

    Mat fileImage = imread(fileName.toStdString());
    cv::resize(fileImage, fileImage, Size(320,240));
    cvtColor(fileImage, grayImage, COLOR_BGR2GRAY);
    cvtColor(fileImage, colorImage, COLOR_BGR2RGB);
}

void MainWindow::saveImage()
{
    timer.stop();
    auto fileName = QFileDialog::getSaveFileName(this,
        tr("Save Image"), "/home/alumno/Imágenes/image.jpg", tr("Image Files (*.png *.jpg *.bmp)"));
    timer.start(30);

    if (ui->colorButton->isChecked())
    {
        Mat aux;
        cvtColor(destColorImage, aux, COLOR_RGB2BGR);
        imwrite(fileName.toStdString(), aux);
    }
    else
        imwrite(fileName.toStdString(), destGrayImage);
}


