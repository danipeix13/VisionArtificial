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

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(mouseSelection(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(mouseClic(QPointF)),this,SLOT(deselectWindow(QPointF)));
    connect(ui->loadBtn,SIGNAL(clicked()),this,SLOT(loadImage()));
    connect(ui->saveBtn,SIGNAL(clicked()),this,SLOT(saveImage()));
    connect(ui->copyChannelsBtn,SIGNAL(clicked()),this,SLOT(copyChannels()));
    connect(ui->copyWindowBtn,SIGNAL(clicked()),this,SLOT(copyWindow()));
    connect(ui->resizeWindowBtn,SIGNAL(clicked()),this,SLOT(resizeWindow()));
    connect(ui->enlargeWindowBtn,SIGNAL(clicked()),this,SLOT(enlargeWindow()));
    connect(visorS,SIGNAL(mouseClic(QPointF)),this,SLOT(getPixelValues(QPointF)));

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


    //En este punto se debe incluir el código asociado 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con el procesamiento de cada captura
    if(ui->warpZoomBtn->isChecked())
        warpZoom();

    //Actualización de los visores

    if(winSelected)
        visorS->drawSquare(QRect(imageWindow.x, imageWindow.y, imageWindow.width,imageWindow.height), Qt::green );

    visorS->update();
    visorD->update();

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

//ENTREGA 1
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

void MainWindow::copyChannels()
{
    if (ui->colorButton->isChecked())
    {
        std::vector<Mat> channels;
        split(colorImage, channels);

        if (!ui->rCheck->isChecked())
            channels[0].setTo(0);
        if (!ui->gCheck->isChecked())
            channels[1].setTo(0);
        if (!ui->bCheck->isChecked())
            channels[2].setTo(0);

        merge(channels, destColorImage);
    }
}

void MainWindow::copyWindow()
{
    if(winSelected)
    {
        Mat copyWindow, destImage;
        int x = (320 - imageWindow.width) / 2, y = (240 - imageWindow.height) / 2;

        destColorImage.setTo(0);
        destGrayImage.setTo(0);

        copyWindow = Mat(colorImage,imageWindow);
        destImage = Mat(destColorImage, Rect(x, y, imageWindow.width, imageWindow.height));
        copyWindow.copyTo(destImage);

        copyWindow = Mat(grayImage,imageWindow);
        destImage = Mat(destGrayImage, Rect(x, y, imageWindow.width, imageWindow.height));
        copyWindow.copyTo(destImage);
    }
}

void MainWindow::resizeWindow()
{
    if(winSelected)
    {
        Mat copyWindow,destImage;

        destColorImage.setTo(0);
        destGrayImage.setTo(0);

        copyWindow = Mat(colorImage,imageWindow);
        cv::resize(copyWindow, copyWindow, Size(320,240));
        copyWindow.copyTo(destColorImage);

        copyWindow = Mat(grayImage,imageWindow);
        cv::resize(copyWindow, copyWindow, Size(320,240));
        copyWindow.copyTo(destGrayImage);
    }
}

void MainWindow::enlargeWindow()
{
    if(winSelected)
    {
        Mat copyWindow, destImage, auxImage;
        float fx = 320. / imageWindow.width, fy = 240. / imageWindow.height, factor;
        int y, x;


        destColorImage.setTo(0);
        destGrayImage.setTo(0);

        if (fx < fy)
           factor = fx;
        else
           factor = fy;

        copyWindow = Mat(colorImage,imageWindow);
        cv::resize(copyWindow, auxImage, Size(), factor, factor);
        x = (320 - auxImage.cols) / 2;
        y = (240 - auxImage.rows) / 2;
        destImage = Mat(destColorImage, Rect(x, y, auxImage.cols, auxImage.rows));
        auxImage.copyTo(destImage);

        copyWindow = Mat(grayImage,imageWindow);
        cv::resize(copyWindow, auxImage, Size(), factor, factor);
        x = (320 - auxImage.cols) / 2;
        y = (240 - auxImage.rows) / 2;
        destImage = Mat(destGrayImage, Rect(x, y, auxImage.cols, auxImage.rows));
        auxImage.copyTo(destImage);
    }
}

void MainWindow::getPixelValues(QPointF point)
{
    Vec3b value;
    String text;

    if(ui->colorButton->isChecked())
    {
        value = colorImage.at<Vec3b>(point.x(), point.y());
        text = "R:" + std::to_string(value[0]) + "  G:" + std::to_string(value[1]) + "  B:" + std::to_string(value[2]);
    }
    else
    {
        value = grayImage.at<Vec3b>(point.x(), point.y());
        text = "GRAY:" + std::to_string(value[0]);
    }

    QToolTip::showText(mapToGlobal(point.toPoint()), QString::fromStdString(text));
}

void MainWindow::warpZoom()
{
    float angle = ui->dial->value();
    int hTranslation = ui->horizontalTranslation->value();
    int vTranslation = ui->verticalTranslation->value();
    int zoom = ui->zoom->value();

    Mat rtMatrix = cv::getRotationMatrix2D(cv::Point2f(160, 120), angle, zoom);

    rtMatrix.at<double>(0, 2) += hTranslation;
    rtMatrix.at<double>(1, 2) += vTranslation;

    cv::warpAffine(colorImage,destColorImage, rtMatrix, Size(320, 240));
    cv::warpAffine(grayImage,destGrayImage, rtMatrix, Size(320, 240));
}

