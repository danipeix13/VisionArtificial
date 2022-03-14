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

    auxMat = Mat();
    lastOption = -1;

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

    connect(pixelTDialog.negativeBtn,SIGNAL(clicked()),this,SLOT(setLutNegative()));
    connect(pixelTDialog.frestyleBtn,SIGNAL(clicked()),this,SLOT(setLutFreestyle()));
    connect(pixelTDialog.resetBtn,SIGNAL(clicked()),this,SLOT(resetTransformationPixel()));

    connect(lFilterDialog.horizontalBtn,SIGNAL(clicked()),this,SLOT(setHorizontalBorders()));
    connect(lFilterDialog.verticalBtn,SIGNAL(clicked()),this,SLOT(setVerticalBorders()));
    connect(lFilterDialog.resetBtn,SIGNAL(clicked()),this,SLOT(resetLinearKernel()));
    connect(lFilterDialog.sharpenBtn,SIGNAL(clicked()),this,SLOT(setSharpenKernel()));
    connect(lFilterDialog.randomBtn,SIGNAL(clicked()),this,SLOT(setRandomKernel()));


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

    Mat aux_image, intensity;
    std::vector<Mat> channels;

    cv::cvtColor(colorImage, aux_image, cv::COLOR_RGB2YUV);
    split(aux_image, channels);

    //En este punto se debe incluir el código asociado con el procesamiento de cada captura
    int option = ui->operationComboBox->currentIndex();
    if (option != lastOption)
        auxMat = Mat();
    lastOption = option;

    Mat src, dst;
    if(ui->colorButton->isChecked())
    {
        src = channels[0];
    }
    else
    {
        src = grayImage;
        dst = destGrayImage;
    }

    //Método que llame al switch
    selectOperation(option,src,dst);

    //Actualización de los visores
     if(!ui->colorButton->isChecked())
     {
         updateHistograms(grayImage, visorHistoS);
         updateHistograms(destGrayImage, visorHistoD);
     }
     else
     {
         updateHistograms(channels[0], visorHistoS);

         std::vector<Mat> dstcanales = {dst,channels[1],channels[2]};
         cv::merge(dstcanales, destColorImage);
         cv::cvtColor(destColorImage, destColorImage, cv::COLOR_YUV2RGB);
         updateHistograms(dst, visorHistoD);
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

void MainWindow::pixelTransformation(Mat src, Mat &dst)
{
    int r0 = 0,
        s0 = pixelTDialog.grayTransformW->item(0, 1)->text().toInt(),
        r1 = pixelTDialog.grayTransformW->item(1, 0)->text().toInt(),
        s1 = pixelTDialog.grayTransformW->item(1, 1)->text().toInt(),
        r2 = pixelTDialog.grayTransformW->item(2, 0)->text().toInt(),
        s2 = pixelTDialog.grayTransformW->item(2, 1)->text().toInt(),
        r3 = 255,
        s3 = pixelTDialog.grayTransformW->item(3, 1)->text().toInt();

    std::vector<uchar> lut = fillLutTable(r0, s0, r1, s1, r2, s2, r3, s3);
//    for(int i = 0; i < 256; i++)
//        qDebug() << i << " - " << lut[i];
    cv::LUT(src, lut, dst);

}

std::vector<uchar> MainWindow::fillLutTable(int r0, int s0, int r1, int s1, int r2, int s2, int r3, int s3)
{
    std::vector<uchar> lut;
    int m, n, i = 0;

    while(i < r1)
    {
        m = (s1 - s0) / (r1 - r0);
        n = s1 - m * r1;
        lut.push_back(m * i + n);
        i++;
    }

    while(i < r2)
    {
        m = (s2 - s1) / (r2 - r1);
        n = s2 - m * r2;
        lut.push_back(m * i + n);
        i++;
    }

    while(i <= r3)
    {
        m = (s3 - s2) / (r3 - r2);
        n = s3 - m * r3;
        lut.push_back(m * i + n);
        i++;
    }

    return lut;
}

void MainWindow::thresholding(Mat src, Mat &dst)
{
    cv::threshold(src, dst, ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
}


void MainWindow::equalize(Mat src, Mat &dst)
{
    cv::equalizeHist(src, dst);
}

void MainWindow::gaussianBlur(Mat src, Mat &dst)
{
    //TODO : Color?
    int w = ui->gaussWidthBox->value();
    cv::GaussianBlur(src, dst, Size(w, w), w/5);
}

void MainWindow::medianBlur(Mat src, Mat &dst)
{
    //TODO : Color?
    cv::medianBlur(src, dst, 3);
}

void MainWindow::dilate(Mat src, Mat &dst)
{
    if (auxMat.empty())
        thresholding(src, dst);
    else
        thresholding(auxMat, dst);
    Mat kernel = Mat();
    cv::dilate(dst, dst, kernel);
    auxMat = dst;
}

void MainWindow::erode(Mat src, Mat &dst)
{
    if (auxMat.empty())
        thresholding(src, dst);
    else
        thresholding(auxMat, dst);
    Mat kernel = Mat();
    cv::erode(dst, dst, kernel);
    auxMat = dst;
}

void MainWindow::linearFilter(Mat src, Mat &dst)
{

    int aa = lFilterDialog.kernelWidget->item(0, 0)->text().toInt(),
        ab = lFilterDialog.kernelWidget->item(0, 1)->text().toInt(),
        ac = lFilterDialog.kernelWidget->item(0, 2)->text().toInt(),
        ba = lFilterDialog.kernelWidget->item(1, 0)->text().toInt(),
        bb = lFilterDialog.kernelWidget->item(1, 1)->text().toInt(),
        bc = lFilterDialog.kernelWidget->item(1, 2)->text().toInt(),
        ca = lFilterDialog.kernelWidget->item(2, 0)->text().toInt(),
        cb = lFilterDialog.kernelWidget->item(2, 1)->text().toInt(),
        cc = lFilterDialog.kernelWidget->item(2, 2)->text().toInt();

    Mat kernel = (Mat_<int>(3, 3) << aa, ab, ac, ba, bb, bc, ca, cb, cc);
    int delta = lFilterDialog.addedVBox->value();
    cv::filter2D(src, dst, -1, kernel, Point(-1, -1), delta);
}

std::vector<Mat> MainWindow::splitColorImage()
{
    Mat aux_image;
    std::vector<Mat> channels;

    cv::cvtColor(colorImage, aux_image, cv::COLOR_RGB2YUV);
    cv::split(aux_image, channels);

    return channels;
}

void MainWindow::mergeColorImage(std::vector<Mat> channels)
{
    Mat aux;
    cv::merge(channels, aux);
    cv::cvtColor(aux, destColorImage, cv::COLOR_YUV2RGB);
}

void MainWindow::selectOperation(int option, Mat src,Mat &dst)
{
    switch(option)
    {
    case 0:
        pixelTransformation(src, dst);
        break;
    case 1:
        thresholding(src, dst);
        break;
    case 2:
        equalize(src, dst);
        break;
    case 3:
        gaussianBlur(src, dst);
        break;
    case 4:
        medianBlur(src, dst);
        break;
    case 5:
        linearFilter(src, dst);
        break;
    case 6: //sobre la misma imagen
        dilate(src, dst);
        break;
    case 7: //sobre la misma imagen
        erode(src, dst);
        break;
    case 8:
        applySeveral(src, dst);
        break;
    default:
        printf("Unimplemented operation");
    }
}
void MainWindow::applySeveral(Mat src, Mat &dst)
{

    int option = 0;
    if(operOrderDialog.firstOperCheckBox->isChecked())
    {
        option = operOrderDialog.operationComboBox1->currentIndex();
        selectOperation(option,src,dst);

        if(operOrderDialog.secondOperCheckBox->isChecked())
        {
            option = operOrderDialog.operationComboBox2->currentIndex();
            selectOperation(option,dst,dst);

            if(operOrderDialog.thirdOperCheckBox->isChecked())
            {
                option = operOrderDialog.operationComboBox3->currentIndex();
                selectOperation(option,dst,dst);

                if(operOrderDialog.fourthOperCheckBox->isChecked())
                {
                    option = operOrderDialog.operationComboBox4->currentIndex();
                    selectOperation(option,dst,dst);
                }
            }
        }
    }
}

void MainWindow::setLutNegative()
{
    pixelTDialog.grayTransformW->item(0, 1)->setText(QString("255"));
    pixelTDialog.grayTransformW->item(1, 1)->setText(QString("170"));
    pixelTDialog.grayTransformW->item(2, 1)->setText(QString("85"));
    pixelTDialog.grayTransformW->item(3, 1)->setText(QString("0"));
}

void MainWindow::setLutFreestyle()
{
    std::cout << "CACA" << std::endl;
    for (int i = 0; i < 4; i++)
        pixelTDialog.grayTransformW->item(i, 1)->setText(QString(std::to_string(rand() % 255).c_str()));
}

void::MainWindow::resetTransformationPixel()
{
    pixelTDialog.grayTransformW->item(0, 1)->setText(QString("0"));
    pixelTDialog.grayTransformW->item(1, 1)->setText(QString("85"));
    pixelTDialog.grayTransformW->item(2, 1)->setText(QString("170"));
    pixelTDialog.grayTransformW->item(3, 1)->setText(QString("255"));
}

void MainWindow::setHorizontalBorders()
{
    lFilterDialog.kernelWidget->item(0,0)->setText(QString("-1"));
    lFilterDialog.kernelWidget->item(0,1)->setText(QString("-2"));
    lFilterDialog.kernelWidget->item(0,2)->setText(QString("-1"));
    lFilterDialog.kernelWidget->item(1,0)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(1,1)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(1,2)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(2,0)->setText(QString("1"));
    lFilterDialog.kernelWidget->item(2,1)->setText(QString("2"));
    lFilterDialog.kernelWidget->item(2,2)->setText(QString("1"));
}

void MainWindow::setVerticalBorders()
{
    lFilterDialog.kernelWidget->item(0,0)->setText(QString("-1"));
    lFilterDialog.kernelWidget->item(0,1)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(0,2)->setText(QString("1"));
    lFilterDialog.kernelWidget->item(1,0)->setText(QString("-2"));
    lFilterDialog.kernelWidget->item(1,1)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(1,2)->setText(QString("2"));
    lFilterDialog.kernelWidget->item(2,0)->setText(QString("-1"));
    lFilterDialog.kernelWidget->item(2,1)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(2,2)->setText(QString("1"));
}

void MainWindow::resetLinearKernel()
{
    lFilterDialog.kernelWidget->item(0,0)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(0,1)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(0,2)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(1,0)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(1,1)->setText(QString("1"));
    lFilterDialog.kernelWidget->item(1,2)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(2,0)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(2,1)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(2,2)->setText(QString("0"));
}

void MainWindow::setSharpenKernel()
{
    lFilterDialog.kernelWidget->item(0,0)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(0,1)->setText(QString("-1"));
    lFilterDialog.kernelWidget->item(0,2)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(1,0)->setText(QString("-1"));
    lFilterDialog.kernelWidget->item(1,1)->setText(QString("5"));
    lFilterDialog.kernelWidget->item(1,2)->setText(QString("-1"));
    lFilterDialog.kernelWidget->item(2,0)->setText(QString("0"));
    lFilterDialog.kernelWidget->item(2,1)->setText(QString("-1"));
    lFilterDialog.kernelWidget->item(2,2)->setText(QString("0"));
}

void MainWindow::setRandomKernel()
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            lFilterDialog.kernelWidget->item(i, j)->setText(QString(std::to_string(rand() % 8 - 3).c_str()));
}



