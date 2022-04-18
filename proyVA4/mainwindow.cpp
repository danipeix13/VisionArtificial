#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cap = new VideoCapture(0);
    cap->set(CAP_PROP_FPS, 30);
    winSelected = false;

    colorImage.create(240,320,CV_8UC3);
    grayImage.create(240,320,CV_8UC1);
    destColorImage.create(240,320,CV_8UC3);
    destColorImage.setTo(0);
    destGrayImage.create(240,320,CV_8UC1);
    destGrayImage.setTo(0);

    visorS = new ImgViewer(&colorImage, ui->imageFrameS);
    visorD = new ImgViewer(&destColorImage, ui->imageFrameD);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(mouseSelection(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(mouseClic(QPointF)),this,SLOT(deselectWindow(QPointF)));
    connect(ui->loadBtn,SIGNAL(clicked()),this,SLOT(loadImage()));
    connect(ui->segmentBtn,SIGNAL(clicked()),this,SLOT(segmentImage()));
    timer.start(30);

    net = cv::dnn::readNetFromCaffe("../proyVA4/fcn/fcn.prototxt", "../proyVA4/fcn/fcn.caffemodel");
    fillColorTable();
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

void MainWindow::loadImage()
{
    timer.stop();
    auto fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "/home/alumno/Imágenes", tr("Image Files (*.png *.jpg *.bmp)"));
    timer.start(30);

    if (fileName == "")
        qDebug() << __FUNCTION__ << "No image loaded";
    else
    {
        Mat fileImage = imread(fileName.toStdString());
        cv::resize(fileImage, fileImage, Size(320,240));
        cvtColor(fileImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(fileImage, colorImage, COLOR_BGR2RGB);
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

void MainWindow::fillColorTable()
{
    String fileName = "../proyVA4/fcn/fcn-colors.txt";
    std::fstream colors;
    colors.open(fileName.c_str(), std::ios::in);
    if(colors.is_open())
    {
        String r, g, b;
        getline(colors, r, ',');
        while(r != "")
        {
            getline(colors, g, ',');
            getline(colors, b);
            std::vector<int> auxVec;
            auxVec.push_back(std::atoi(r.c_str()));
            auxVec.push_back(std::atoi(g.c_str()));
            auxVec.push_back(std::atoi(b.c_str()));
            colorTable.push_back(auxVec);
            getline(colors, r, ',');
        }
    }else
        qDebug() << "PRIMO, NO SABEMOS LEER DE UN PUTO FILE O Q?";
    for (std::vector<int> category : colorTable){
        for(int color : category)
            std::cout << color << " ";
        std::cout << "\n";
    }
    std::cout << "CACA" << std::endl;
}

void MainWindow::segmentImage()
{
    Mat auxImage;
    Scalar meanRgbValue = cv::mean(colorImage);
    Size imgSize = Size(colorImage.rows, colorImage.cols);
    dnn::blobFromImage(colorImage, auxImage, 1.0, imgSize, meanRgbValue, true);

    net.setInput(auxImage);

    Mat output;
    net.forward(output);

    //Mat segmentedImage = processOutput(output);



}

Mat MainWindow::processOutput(Mat output)
{
    Mat categories;
//    for(int i = 0; i < output.rows; i++)
//    {
//        for(int j = 0; j < output.cols; j++)
//        {
//            int best = 0;
//            for(int k = 0; k < 21; i++)
//            {
//                std::cout << output[Mat()][Mat()];
////                int catValue = output[1][k][i][j];
//                if(best > catValue)
//                {
//                    best = catValue;
//                }
//            }
//            categories[i][j] = best;
//        }
//    }
    return categories;
}


