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
    combinedImage.create(480,640,CV_8UC3);
    combinedImage.setTo(0);

    visorS = new ImgViewer(&colorImage, ui->imageFrameS);
    visorD = new ImgViewer(&destColorImage, ui->imageFrameD);
    visorC = new ImgViewer(&combinedImage, combineDialog.combiningFrm);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(mouseSelection(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(mouseClic(QPointF)),this,SLOT(deselectWindow(QPointF)));
    connect(ui->loadBtn,SIGNAL(clicked()),this,SLOT(loadImage()));
    connect(ui->segmentBtn,SIGNAL(clicked()),this,SLOT(segmentImage()));
    connect(ui->combineBtn,SIGNAL(clicked()),&combineDialog,SLOT(show()));

    // CombineDialog
    connect(combineDialog.loadBtn,SIGNAL(clicked()),this,SLOT(loadBackground()));
    connect(combineDialog.saveBtn,SIGNAL(clicked()),this,SLOT(saveImage()));
    connect(combineDialog.pasteBtn,SIGNAL(clicked()),this,SLOT(pasteImage()));
    connect(combineDialog.closeBtn,SIGNAL(clicked()),&combineDialog,SLOT(hide()));

    connect(ui->selectCategoriesBtn,SIGNAL(clicked()),&lFilterDialog,SLOT(show()));
    connect(lFilterDialog.pushButton,SIGNAL(clicked()),this,SLOT(updateCategories()));
    connect(visorC,SIGNAL(mouseClic(QPointF)),this,SLOT(copyObject(QPointF)));

    timer.start(30);

    net = cv::dnn::readNetFromCaffe("../proyVA4/fcn/fcn.prototxt", "../proyVA4/fcn/fcn.caffemodel");
    fillColorTable();
    readCategories();
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
    if(ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> colorImage;
        cv::resize(colorImage, colorImage, Size(320,240));
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);

        ui->imgHSpb->setValue(240);
        ui->imgWSpb->setValue(320);
    }

    if(winSelected)
        visorS->drawSquare(QRect(imageWindow.x, imageWindow.y, imageWindow.width,imageWindow.height), Qt::green );

    visorS->update();
    visorD->update();
    visorC->update();
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
    visorC->setImage(&backgroundImage);
}

void MainWindow::loadImage()
{
    timer.stop();
    auto fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "/home/alumno/Im??genes", tr("Image Files (*.png *.jpg *.bmp *.jpeg *.webp)"));
    timer.start(30);

    if (fileName == "")
        qDebug() << __FUNCTION__ << "No image loaded";
    else
    {
        Mat fileImage = imread(fileName.toStdString());
        Size imgSize = adjustSize(fileImage);
        ui->imgHSpb->setValue(imgSize.height);
        ui->imgWSpb->setValue(imgSize.width);
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
            Vec3b auxVec(std::atoi(r.c_str()), std::atoi(g.c_str()), std::atoi(b.c_str()));
            colorTable.push_back(auxVec);
            getline(colors, r, ',');
        }
    }
    else
        qDebug() << "The file is closed";
}

Size MainWindow::adjustSize(Mat fileImage)
{
    int height = fileImage.rows,
         width = fileImage.cols;

    if(height > width && height > 500)
    {
        width = 500 / (height * 1.0 / width);
        height = 500;
    }
    else if(width > height && width > 500)
    {
        height = 500 / (width * 1.0 / height);
        width = 500;
    }

    return Size(width, height);
}

void MainWindow::segmentImage()
{
    int height = ui->imgHSpb->value(),
         width = ui->imgWSpb->value();
    Scalar meanRgbValue = cv::mean(colorImage);
    Size imgSize = Size(width, height);
    Mat auxImage = dnn::blobFromImage(colorImage, 1.0, imgSize, meanRgbValue, true);

    net.setInput(auxImage);

    Mat output = net.forward();

    Mat segmentedImage = processOutput(output, height, width);

    //qDebug() << "segmentedImage SIZE" << segmentedImage.rows << segmentedImage.cols;
    cv::resize(segmentedImage, segmentedImage, Size(320, 240), INTER_NEAREST);
    //qDebug() << "segmentedImage NEW SIZE" << segmentedImage.rows << segmentedImage.cols;

    Mat result = mixImages(segmentedImage);
    //qDebug() << "MIX IMAGES";
    result.copyTo(destColorImage);
    //qDebug() << "COPYTO";
}

Mat MainWindow::processOutput(Mat output, int height, int width)
{
    Mat categories = Mat(height, width, CV_8UC3);
    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            int bestCatIndex = 0;
            for(int k = 0; k < 21; k++)
            {
                float catValue = output.at<float>(Vec<int,4>(0,k,i,j));
                float bestValue = output.at<float>(Vec<int,4>(0,bestCatIndex,i,j));
                if(bestValue < catValue)
                {
                    bestCatIndex = k;
                }
            }
            if(availableCategories[bestCatIndex])
                categories.at<Vec3b>(Point(j, i)) = colorTable.at(bestCatIndex);
            else
                categories.at<Vec3b>(Point(j, i)) = {0, 0, 0};
        }
    }
    return categories;
}

Mat MainWindow::mixImages(Mat input)
{
    float p = ui->catColorBar->value() / 100.0;
    for(int i = 0; i < input.rows; i++)
        for(int j = 0; j < input.cols; j++)
        {
            input.at<Vec3b>(Point(j, i)) *= p;
            input.at<Vec3b>(Point(j, i)) += (1 - p) * colorImage.at<Vec3b>(Point(j, i));
        }
    return input;
}

void MainWindow::readCategories()
{
    String fileName = "../proyVA4/fcn/fcn-classes.txt";
    std::fstream colors;
    colors.open(fileName.c_str(), std::ios::in);

    if(colors.is_open())
    {
        std::string name;
        getline(colors, name);
        while(name != "")
        {
            lFilterDialog.listWidget->addItem(QString(name.c_str()));
            lFilterDialog.listWidget->item(lFilterDialog.listWidget->count()-1)->setCheckState(Qt::Checked);
            availableCategories.push_back(true);
            getline(colors, name);
        }
        availableCategories.resize(availableCategories.size());
    }
    else
        qDebug() << "The file is closed";
}


void MainWindow::updateCategories()
{
    for (int i = 0; i < availableCategories.size(); i++)
    {
        availableCategories[i] = lFilterDialog.listWidget->item(i)->checkState() == Qt::Checked;
        qDebug() << i << availableCategories[i];
    }
    lFilterDialog.hide();
}

void MainWindow::loadBackground()
{
    timer.stop();
    auto fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "/home/alumno/Im??genes", tr("Image Files (*.png *.jpg *.bmp *.jpeg *.webp)"));
    timer.start(30);

    if (fileName == "")
        qDebug() << __FUNCTION__ << "No image loaded";
    else
    {
        Mat fileImage = imread(fileName.toStdString());
        Size imgSize = adjustSize(fileImage);
        ui->imgHSpb->setValue(imgSize.height);
        ui->imgWSpb->setValue(imgSize.width);
        cv::resize(fileImage, fileImage, Size(640, 480));
        cvtColor(fileImage, combinedImage, COLOR_BGR2RGB);
        combinedImage.copyTo(backgroundImage);
    }
}

void MainWindow::saveImage()
{
    timer.stop();
    auto fileName = QFileDialog::getSaveFileName(this,
        tr("Save Image"), "/home/alumno/Im??genes/image.jpg", tr("Image Files (*.png *.jpg *.bmp)"));
    timer.start(30);

    Mat aux;
    cvtColor(combinedImage, aux, COLOR_RGB2BGR);
    imwrite(fileName.toStdString(), aux);
}

void MainWindow::pasteImage()
{
    winSelected = false;
    combinedImage.copyTo(backgroundImage);
}

void MainWindow::copyObject(QPointF position)
{
    if(winSelected)
    {
        Mat objectWindow, targetInBackground, mask;

        backgroundImage.copyTo(combinedImage);

        objectWindow = Mat(colorImage,imageWindow);
        mask = Mat(destColorImage,imageWindow);
        cvtColor(mask, mask, COLOR_RGB2GRAY);

        int x = position.x(), y = position.y();
        if(x + objectWindow.cols > backgroundImage.cols)
            x = backgroundImage.cols - objectWindow.cols;
        if(y + objectWindow.rows > backgroundImage.rows)
            y = backgroundImage.rows - objectWindow.rows;

        targetInBackground = Mat(combinedImage, Rect(x, y, imageWindow.width, imageWindow.height));
        objectWindow.copyTo(targetInBackground, mask);
    }
}
