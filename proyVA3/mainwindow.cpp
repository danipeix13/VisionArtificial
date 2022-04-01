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
    std::cout << images.size() << std::endl;

    ui->boxObj->setItemText(ui->boxObj->currentIndex(),ui->boxObj->currentText());

    Mat matAux = copyWindow();
    Mat detectionMat;
    std::cout << "INDICE" << ui->boxObj->currentIndex() << std::endl;

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

        qDebug() << invalid;
        if (invalid)
            std::cout << "NO KP WERE DETECTED" << std::endl;
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
    //Hasta aqui bien uwu
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
    Mat imageDesc;
    std::vector<KeyPoint> imageKp;

    // OBTENER LOS MATCHES
    orbDetector->detectAndCompute(grayImage, Mat(), imageKp, imageDesc);
    if(!imageKp.empty() and !imageDesc.empty())
    {
        qDebug() << "IMAGAE KP AND IMAGEDESCK HAS VALUES";
        std::vector<std::vector<DMatch>> matches;
        matcher->knnMatch(imageDesc, matches, 3);
        if (matches.size() > 0)
        {
            // ORDENAR LOS MATCHES
            std::vector<std::vector<std::vector<DMatch>>> ordered_matches;
            ordered_matches.resize(3);
            for(int i = 0; i < ordered_matches.size(); i++)
                ordered_matches[i].resize(3);

            qDebug() << "236";

            for (std::vector<DMatch> vec : matches)
                for(DMatch m: vec)
                    if (m.distance <= 30)
                    {
                        int objeto = collect2object[m.imgIdx / 3],
                                escala = m.imgIdx % 3;
                        ordered_matches[objeto][escala].push_back(m);
                    }

            qDebug() << "247";

            // ELEGIR MEJOR MACH
            /*std::vector<std::vector<DMatch>> bestMatches;
        for(int objeto = 0; objeto < 3; objeto++)
                bestMatches.push_back(std::ranges::max_element(ordered_matches[objeto], [this](auto a, auto b){return a.size() < b.size();}));
        std::vector<DMatch> bestMatch = std::ranges::max_element(bestMatches, [this](auto a, auto b){return a.size() < b.size();});*/

            int maxMatchsNumber = 0, best_x, best_y;
            for(int i = 0; i < 3; i++)
                for(int j = 0; j < 3; j++)
                {
                    //                qDebug() << "ordered_matches" << ordered_matches[i][j].size();
                    //                qDebug() << "MaxMatchsNumber" << maxMatchsNumber;

                    if (ordered_matches[i][j].size() > maxMatchsNumber)
                    {
                        maxMatchsNumber = ordered_matches[i][j].size();
                        best_x = i;
                        best_y = j;
                        qDebug() << "bESTmATCH";
                    }
                }

            qDebug() << "271";

            if(ordered_matches[best_x][best_y].size() > 10)
            {
                qDebug() << "DSPS DE ORDENAR";

                // GENERAR CORRESPONDENCIA DE PUNTOS
                qDebug() << "GENERAR CORRESPONDENCIA DE PUNTOS";
                std::vector<Point2f> imagePoints, objectPoints;
                qDebug() << "WTF";
                std::vector<DMatch> bestMatch = ordered_matches[best_x][best_y];
                qDebug() << "MatchSize" << bestMatch.size();


                for(DMatch m : bestMatch)
                {
                    imagePoints.push_back(imageKp[m.queryIdx].pt);
                    objectPoints.push_back(objectKP[best_x][best_y][m.trainIdx].pt);
                }

                // OBTENER Y APLICAR HOMOGRAFIA
                qDebug() << "OBTENER Y APLICAR HOMOGRAFIA";
                Mat H = findHomography(objectPoints, imagePoints, LMEDS);

                if(!H.empty())
                {
                    qDebug() << "DSPS de homografia";

                    Mat image = images[best_x];
                    int h = image.rows * scaleFactors[best_y], w = image.cols * scaleFactors[best_y];

                    qDebug() << "H:" << h << "W:" << w;
                    std::vector<Point2f> imageCorners, objectCorners = {Point2f(0, 0), Point2f(w-1, 0), Point2f(w-1, h-1), Point2f(0, h-1)};
                    perspectiveTransform(objectCorners, imageCorners, H);

                    // PINTAR RESULTADO
                    qDebug() << "PINTAR RESULTADO";
                    std::initializer_list<QPoint> object_draw = {QPoint(imageCorners[0].x, imageCorners[0].y),
                                                                 QPoint(imageCorners[1].x, imageCorners[1].y),
                                                                 QPoint(imageCorners[2].x, imageCorners[2].y),
                                                                 QPoint(imageCorners[3].x, imageCorners[3].y),
                                                                 QPoint(imageCorners[0].x, imageCorners[0].y)};

                    //        QPolygonF object_polygon = QPolygonF(QVector<QPointF>(object_draw));
                    visorS->drawPolyLine(QVector<QPoint>(object_draw), Qt::red);
                }
            }
            else
                qInfo() << "NOT ENOUGH MATCHES";
        }
        else
            qInfo() << "Matches Size = 0";
    }
}
// TODO
/*
    Para calcular la homografñia se necesitan un mínimo de puntos: comprobar
    For each raro
*/
