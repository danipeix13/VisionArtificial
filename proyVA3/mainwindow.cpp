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

    ui->boxObj->addItem(QString("[EMPTY]"));
    ui->boxObj->addItem(QString("[EMPTY]"));
    ui->boxObj->addItem(QString("[EMPTY]"));

    connect(ui->boxObj,SIGNAL(currentIndexChanged(int)),this,SLOT(showImage(int)));

    timer.start(30);

    orbDetector = ORB::create();
    matcher = BFMatcher::create(NORM_HAMMING);

    images.resize(3);
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
    collectionMatching();

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
        destGrayImage.setTo(0);
        return Mat(grayImage,imageWindow);
    }
    else
        return Mat();
}

void MainWindow::addObject()
{
    //XAux: std::vector<std::vector<X>>
    //XScale: std::vector<X>
    qDebug() << __FUNCTION__ << "Tamaño de images:" << images.size();

    ui->boxObj->setItemText(ui->boxObj->currentIndex(),ui->boxObj->currentText());

    Mat matAux = copyWindow();
    Mat detectionMat;
    qDebug() << __FUNCTION__ << "Índice:" << ui->boxObj->currentIndex();

    if (!matAux.empty())
    {
        matAux.copyTo(images[ui->boxObj->currentIndex()]);

        std::vector<KeyPoint> kpScale;
        std::vector<std::vector<KeyPoint>>kpObject;
        Mat descScale;
        std::vector<Mat> descObject;

        bool invalid = false;
        for (float factor : scaleFactors)
        {
            if (invalid)
                break;

            cv::resize(matAux, detectionMat, Size(), factor, factor);
            orbDetector->detectAndCompute(detectionMat, Mat(), kpScale, descScale);

            if (kpScale.size() > 0)
            {
                kpObject.push_back(kpScale);
                descObject.push_back(descScale);
            }
            else
                invalid = true;
        }
        if (invalid)
            qDebug() << __FUNCTION__ << "No se detectaron KeyPoints";
        else
        {
            int x = (320 - imageWindow.width) / 2, y = (240 - imageWindow.height) / 2;
            Mat destImage = Mat(destGrayImage, Rect(x, y, imageWindow.width, imageWindow.height));
            matAux.copyTo(destImage);
            objectKP.insert(objectKP.begin()+ui->boxObj->currentIndex(),kpObject);
            objectDesc.insert(objectDesc.begin()+ui->boxObj->currentIndex(),descObject);
            collect2object.push_back(ui->boxObj->currentIndex());
            matcher->clear();
            for (auto &&element : objectDesc)
                matcher->add(element);
            winSelected = false;
        }
    }
}

void MainWindow::deleteObject()
{
    Mat mat = Mat();
    mat.copyTo(images[ui->boxObj->currentIndex()]);
    destGrayImage.setTo(0);
    remove(collect2object.begin(), collect2object.end(), ui->boxObj->currentIndex());
    ui->boxObj->setItemText(ui->boxObj->currentIndex(),"[EMPTY]");
    objectKP[ui->boxObj->currentIndex()].clear();
    objectDesc[ui->boxObj->currentIndex()].clear();
    matcher->clear();
    for (auto &&element : objectDesc)
        matcher->add(element);
}

void MainWindow::showImage(int index)
{
    Mat currentObject = images[index];
    destGrayImage.setTo(0);

    if(!currentObject.empty())
    {
        int x = (320 - currentObject.cols) / 2, y = (240 - currentObject.rows) / 2;
        currentObject.copyTo(Mat(destGrayImage, Rect(x, y, currentObject.cols, currentObject.rows)));
    }
}

void MainWindow::collectionMatching()
{
    qDebug() << __FUNCTION__ << "Detectar";
    Mat imageDesc;
    std::vector<KeyPoint> imageKp;
    orbDetector->detectAndCompute(grayImage, Mat(), imageKp, imageDesc);

    if(!imageKp.empty() and !imageDesc.empty())
    {
        qDebug() << __FUNCTION__ << "ImageKp y imageDesc tienen valor";
        std::vector<std::vector<DMatch>> matches;
        matcher->knnMatch(imageDesc, matches, 3);

        if (matches.size() > 0)
        {
            std::vector<std::vector<std::vector<DMatch>>> ordered_matches = orderMatches(matches);

            int bestObject, bestScale;
            bestMatch(ordered_matches, bestObject, bestScale);

            if(ordered_matches[bestObject][bestScale].size() > 10)
            {
                qDebug() << __FUNCTION__ << "Hay más de 10 matches";

                std::vector<Point2f> imagePoints, objectPoints;

                pointsCorrespondence(ordered_matches[bestObject][bestScale], imageKp, bestObject, bestScale, imagePoints, objectPoints);

                std::vector<Point2f> imageCorners = getAndApplyHomography(imagePoints, objectPoints, bestObject, bestScale);

                paintResult(imageCorners);
            }
            else
                qInfo() << __FUNCTION__ << "No hay más de 10 matches";
        }
        else
            qInfo() << __FUNCTION__ << "No hay matches";
    }
    else
        qInfo() << __FUNCTION__ << "imageKP o imageDesc está vacío";
}

std::vector<std::vector<std::vector<DMatch>>> MainWindow::orderMatches(std::vector<std::vector<DMatch>> matches)
{
    qDebug() << __FUNCTION__ << "Entra";
    std::vector<std::vector<std::vector<DMatch>>> ordered_matches;
    ordered_matches.resize(3);
    for(int i = 0; i < ordered_matches.size(); i++)
        ordered_matches[i].resize(3);

    qDebug() << __FUNCTION__ << "Ordena";

    for (std::vector<DMatch> vec : matches)
        for(DMatch m: vec)
            if (m.distance <= 30)
            {
                int objeto = collect2object[m.imgIdx / 3],
                        escala = m.imgIdx % 3;
                ordered_matches[objeto][escala].push_back(m);
            }

    qDebug() << __FUNCTION__ << "Sale";
    return ordered_matches;
}

void MainWindow::bestMatch(std::vector<std::vector<std::vector<DMatch>>> ordered_matches, int &bestObject, int &bestScale)
{
    qDebug() << __FUNCTION__ << "Entra";
    int maxMatchsNumber = 0;
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
        {
            // qDebug() << "ordered_matches" << ordered_matches[i][j].size();
            // qDebug() << "MaxMatchsNumber" << maxMatchsNumber;

            if (ordered_matches[i][j].size() > maxMatchsNumber)
            {
                maxMatchsNumber = ordered_matches[i][j].size();
                bestObject = i;
                bestScale = j;
                qDebug() << __FUNCTION__ << "Actualiza el mejor match";
            }
        }
    qDebug() << __FUNCTION__ << "Sale";
    /*std::vector<std::vector<DMatch>> bestMatches;
    for(int objeto = 0; objeto < 3; objeto++)
        bestMatches.push_back(std::ranges::max_element(ordered_matches[objeto], [this](auto a, auto b){return a.size() < b.size();}));
    std::vector<DMatch> bestMatch = std::ranges::max_element(bestMatches, [this](auto a, auto b){return a.size() < b.size();});*/
}

void MainWindow::pointsCorrespondence(std::vector<DMatch> bestMatch, std::vector<KeyPoint> imageKp, int bestObject, int bestScale,
                                      std::vector<Point2f> &imagePoints, std::vector<Point2f> &objectPoints)
{
    qDebug() << __FUNCTION__ << "Entra";
    qDebug() << __FUNCTION__ << "Tamaño mejor match:" << bestMatch.size();

    for(DMatch m : bestMatch)
    {
        imagePoints.push_back(imageKp[m.queryIdx].pt);
        objectPoints.push_back(objectKP[bestObject][bestScale][m.trainIdx].pt);
    }
    qDebug() << __FUNCTION__ << "Sale";
}

std::vector<Point2f> MainWindow::getAndApplyHomography(std::vector<Point2f> imagePoints, std::vector<Point2f> objectPoints, int bestObject, int bestScale)
{
    qDebug() << __FUNCTION__ << "Entra";
    Mat H = findHomography(objectPoints, imagePoints, LMEDS);
    qDebug() << __FUNCTION__ << "Homografía obtenida";

    Mat image = images[bestObject];
    int h = image.rows * scaleFactors[bestScale], w = image.cols * scaleFactors[bestScale];

    qDebug() << __FUNCTION__ << "H:" << h << "W:" << w;
    std::vector<Point2f> imageCorners, objectCorners = {Point2f(0, 0), Point2f(w-1, 0), Point2f(w-1, h-1), Point2f(0, h-1)};
    qDebug() << __FUNCTION__ << "Obtiene los puntos";
    perspectiveTransform(objectCorners, imageCorners, H);
    qDebug() << __FUNCTION__ << "Sale";
    return imageCorners;
}

void MainWindow::paintResult(std::vector<Point2f> imageCorners)
{
    qDebug() << __FUNCTION__ << "Entra";
    std::initializer_list<QPoint> object_draw = {QPoint(imageCorners[0].x, imageCorners[0].y),
                                                 QPoint(imageCorners[1].x, imageCorners[1].y),
                                                 QPoint(imageCorners[2].x, imageCorners[2].y),
                                                 QPoint(imageCorners[3].x, imageCorners[3].y),
                                                 QPoint(imageCorners[0].x, imageCorners[0].y)};
    visorS->drawPolyLine(QVector<QPoint>(object_draw), Qt::red);
    qDebug() << __FUNCTION__ << "Sale";
}
