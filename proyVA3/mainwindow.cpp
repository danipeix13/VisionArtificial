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
    connect(visorS,SIGNAL(mouseSelection(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(mouseClic(QPointF)),this,SLOT(deselectWindow(QPointF)));

    connect(ui->addObjBtn,SIGNAL(clicked()),this,SLOT(addObject()));
    connect(ui->delObjBtn,SIGNAL(clicked()),this,SLOT(deleteObject()));

    ui->boxObj->addItem(QString("(EMPTY)"));
    ui->boxObj->addItem(QString("(EMPTY)"));
    ui->boxObj->addItem(QString("(EMPTY)"));

    connect(ui->boxObj,SIGNAL(currentIndexChanged(int)),this,SLOT(showImage(int)));

    timer.start(30);

    orbDetector = ORB::create();
    matcher = BFMatcher::create(NORM_HAMMING);

    images.resize(3);

    //ACTUALIZAR COLECCION: BORRAR ENTERA (matcher->clear() y for add(objectDest[i]))
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

Mat MainWindow::copyWindow()
{
    if(winSelected)
    {
        Mat copyWindow, destImage;
        int x = (320 - imageWindow.width) / 2, y = (240 - imageWindow.height) / 2;

        destGrayImage.setTo(0);

        copyWindow = Mat(grayImage,imageWindow);
        std::cout << "1"<< std::endl;
        destImage = Mat(destGrayImage, Rect(x, y, imageWindow.width, imageWindow.height));//peta segunda imagen
        std::cout << "2"<< std::endl;
        copyWindow.copyTo(destImage);
        std::cout << "3"<< std::endl;
        return destImage;
    }
    else
        return Mat();
}

void MainWindow::addObject()
{
    //XAux: std::vector<std::vector<X>>
    //XScale: std::vector<X>
    std::cout << images.size() << std::endl;

    ui->boxObj->setItemText(ui->boxObj->currentIndex(),ui->boxObj->currentText());

    std::cout << "before copywindow"<< std::endl;
    Mat matAux = copyWindow();
    std::cout << ui->boxObj->currentIndex() << std::endl;

    //images.push_back(matAux);

    if (!matAux.empty())
    {
        images[ui->boxObj->currentIndex()] = matAux;
        std::vector<float> scaleFactors = {0.75, 1.0, 1.25};

        std::vector<KeyPoint> kpScale;
        std::vector<std::vector<KeyPoint>>kpObject;
        Mat descScale;
        std::vector<Mat> descObject;

        bool invalid = false;
        for (float factor : scaleFactors)
        {
            if (invalid)
                break;
            cv::resize(matAux, matAux, Size(), factor, factor);
            orbDetector->detectAndCompute(matAux, Mat(), kpScale, descScale);
            if (kpScale.size() > 0)
            {
                kpObject.push_back(kpScale);
                descObject.push_back(descScale);
            }
            else
                invalid = true;
        }
        if (invalid)
        {
            std::cout << "NO KP WERE DETECTED" << std::endl;
            return;
        }

        //int objectIndex = ui->boxObj->currentIndex();
        objectKP.push_back(kpObject);
        objectDesc.push_back(descObject);

        matcher->clear();
        for (auto &&element : objectDesc)
            matcher->add(element);
        std::cout << "ITEM INSERTED" << std::endl;
    }
}

void MainWindow::deleteObject()
{

}

void MainWindow::showImage(int index)
{
    std::cout << "CACA" << std::endl;
    images.at(index).copyTo(destGrayImage);
}


