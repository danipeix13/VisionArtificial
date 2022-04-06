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

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    //connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(visorS,SIGNAL(mouseSelection(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(mouseClic(QPointF)),this,SLOT(deselectWindow(QPointF)));

    connect(ui->addObjBtn,SIGNAL(clicked()),this,SLOT(addObject()));
    connect(ui->delObjBtn,SIGNAL(clicked()),this,SLOT(deleteObject()));

    ui->boxObj->addItem(QString("[EMPTY]"));
    ui->boxObj->addItem(QString("[EMPTY]"));
    ui->boxObj->addItem(QString("[EMPTY]"));

    connect(ui->boxObj,SIGNAL(currentIndexChanged(int)),this,SLOT(showImage(int)));

    timer.start(500);

    orbDetector = ORB::create();
    matcher = BFMatcher::create(NORM_HAMMING);

    objectDesc.resize(3);
    objectKP.resize(3);
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
        qDebug() << __FUNCTION__ << "Copia realizada";
        std::vector<KeyPoint> kpScale;
        std::vector<std::vector<KeyPoint>>kpObject;
        Mat descScale;
        std::vector<Mat> descObject;
        bool invalid = false;

        for (float factor : scaleFactors)
        {
            qDebug() << __FUNCTION__ << "SUSMUERTOS";
            if (invalid)
                break;

            cv::resize(matAux, detectionMat, Size(), factor, factor);
            orbDetector->detectAndCompute(detectionMat, Mat(), kpScale, descScale);

            if (kpScale.size() > 0)
            {
                qDebug() << __FUNCTION__ << "SUSMUERTISIMOS EN PATINETE";
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
            qDebug() << __FUNCTION__ << "Q es válido cojones";
            int x = (320 - imageWindow.width) / 2, y = (240 - imageWindow.height) / 2;
            Mat destImage = Mat(destGrayImage, Rect(x, y, imageWindow.width, imageWindow.height));
            qDebug() << __FUNCTION__ << "Va a explotar";
            matAux.copyTo(destImage);
            qDebug() << __FUNCTION__ << "No explotes x favor";
            objectKP[ui->boxObj->currentIndex()] = kpObject;
            objectDesc[ui->boxObj->currentIndex()] = descObject;
            qDebug() << __FUNCTION__ << "NIIGU!?" << objectKP.size() << objectDesc.size();
            collect2object.push_back(ui->boxObj->currentIndex());
            qDebug() << __FUNCTION__ << "Ha explotado";
            matcher->clear();
            for (auto &&element : objectDesc)
                matcher->add(element);
            winSelected = false;
            qDebug() << __FUNCTION__ << "Suisidasión";
        }
    }
    else
        qDebug() << __FUNCTION__ << "Empty mat";
}

void MainWindow::deleteObject()
{
    if(!images[ui->boxObj->currentIndex()].empty())
    {
        Mat mat = Mat();
        mat.copyTo(images[ui->boxObj->currentIndex()]);
        destGrayImage.setTo(0);
        remove(collect2object.begin(), collect2object.end(), ui->boxObj->currentIndex());
        ui->boxObj->setItemText(ui->boxObj->currentIndex(),"[EMPTY]");
        objectKP[ui->boxObj->currentIndex()] = std::vector<std::vector<KeyPoint>>();
        objectDesc[ui->boxObj->currentIndex()] = std::vector<Mat>();
        matcher->clear();
        for (auto &&element : objectDesc)
            matcher->add(element);
    }

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

            std::vector<int> bestObject, bestScale;
            std::vector<Match> bestMatches;
            bestMatches.resize(3);
            bestMatch(ordered_matches, bestMatches);

            for(Match match: bestMatches)
            {
                qDebug() << __FUNCTION__ << "Objeto" << match.i;
                if (match.value.size() > 0)
                {
                    qDebug() << __FUNCTION__ << "Tiene cosos";
                    int objeto = match.i,  escala = match.j;
                    QColor color = colors.at(objeto);

                    if(match.value.size() > 20)
                    {
                        qDebug() << __FUNCTION__ << "Hay más de 10 matches";

                        std::vector<Point2f> imagePoints, objectPoints;
                        pointsCorrespondence(match.value, imageKp, objeto, escala, imagePoints, objectPoints);

                        std::vector<Point2f> imageCorners;
                        if(getAndApplyHomography(imagePoints, objectPoints, objeto, escala, imageCorners)){
                            paintResult(imageCorners, ui->boxObj->itemText(objeto), color);
                            for(auto &&corner: imageCorners)
                                qDebug() << corner.x << corner.y;
                        }else
                            qInfo() << __FUNCTION__ << "HOMOGRAFÍA DE MIERDA :')";

                    }
                    else
                        qInfo() << __FUNCTION__ << "No hay más de 10 matches";
                }
                else
                    qDebug() << __FUNCTION__ << "NO tiene cosos";
            }

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
        ordered_matches[i].resize(N_SCALES);

    qDebug() << __FUNCTION__ << "Ordena";

    for (std::vector<DMatch> vec : matches)
        if(!vec.empty())
            for(DMatch m: vec)
                if (m.distance <= 50)
                {
                    int objeto = collect2object[m.imgIdx / N_SCALES],
                            escala = m.imgIdx % N_SCALES;
                    ordered_matches[objeto][escala].push_back(m);
                }

    qDebug() << __FUNCTION__ << "Sale";
    return ordered_matches;
}

void MainWindow::bestMatch(std::vector<std::vector<std::vector<DMatch>>> ordered_matches, std::vector<Match> &bestMatches)
{
    qDebug() << __FUNCTION__ << "Entra";

    for(int i = 0; i < images.size(); i++)
    {
        std::vector<std::vector<DMatch>> escala = ordered_matches[i];
        int bestScaleMatch = 0;
        for (int j = 0; j < N_SCALES; j++)
        {
            if(escala[j].size() > escala[bestScaleMatch].size())
            {
                bestScaleMatch = j;
            }
        }
        bestMatches[i] = Match{ordered_matches[i][bestScaleMatch], i, bestScaleMatch};
    }

    qDebug() << __FUNCTION__ << "Sale";
}

void MainWindow::pointsCorrespondence(std::vector<DMatch> bestMatch, std::vector<KeyPoint> imageKp, int bestObject, int bestScale,
                                      std::vector<Point2f> &imagePoints, std::vector<Point2f> &objectPoints)
{
    qDebug() << __FUNCTION__ << "Entra";
    qDebug() << __FUNCTION__ << "Tamaño mejor match:" << bestMatch.size();

    for(DMatch m : bestMatch)
    {
        imagePoints.push_back(imageKp[m.queryIdx].pt);
        qDebug() << __FUNCTION__ << "MECAGOENSUSMUERTOS; PILAR AIUDA";
        qDebug() << bestObject << bestScale << m.trainIdx;
        qDebug() << objectKP.size();
        qDebug() << objectKP[bestObject].size();
        qDebug() << objectKP[bestObject][bestScale].size();
        qDebug() << objectKP[bestObject][bestScale][m.trainIdx].pt.x << objectKP[bestObject][bestScale][m.trainIdx].pt.y;
        objectPoints.push_back(objectKP[bestObject][bestScale][m.trainIdx].pt);
        qDebug() << __FUNCTION__ << "Me MOLI :((((";
    }
    qDebug() << __FUNCTION__ << "Sale";
}

bool MainWindow::getAndApplyHomography(std::vector<Point2f> imagePoints, std::vector<Point2f> objectPoints, int bestObject, int bestScale, std::vector<Point2f> &imageCorners)
{
    qDebug() << __FUNCTION__ << "Entra";
    if(objectPoints.size() > 10 && imagePoints.size() > 10)
    {
        Mat H = findHomography(objectPoints, imagePoints, LMEDS);
        qDebug() << __FUNCTION__ << "Homografía obtenida";

        if(!H.empty())
        {
            Mat image = images[bestObject];
            int h = image.rows * scaleFactors[bestScale], w = image.cols * scaleFactors[bestScale];

            qDebug() << __FUNCTION__ << "H:" << h << "W:" << w;
            std::vector<Point2f> objectCorners = {Point2f(0, 0), Point2f(w-1, 0), Point2f(w-1, h-1), Point2f(0, h-1)};
            qDebug() << __FUNCTION__ << "Obtiene los puntos";
            perspectiveTransform(objectCorners, imageCorners, H);
            qDebug() << __FUNCTION__ << "Sale";
            return true;
        }
        else
            qDebug() << __FUNCTION__ << "Homografia vacia";
    }
    return false;
}

void MainWindow::paintResult(std::vector<Point2f> imageCorners, QString name, QColor color)
{
    qDebug() << __FUNCTION__ << "Entra";
    std::initializer_list<QPoint> object_draw = {QPoint(imageCorners[0].x, imageCorners[0].y),
                                                 QPoint(imageCorners[1].x, imageCorners[1].y),
                                                 QPoint(imageCorners[2].x, imageCorners[2].y),
                                                 QPoint(imageCorners[3].x, imageCorners[3].y),
                                                 QPoint(imageCorners[0].x, imageCorners[0].y)};
    visorS->drawPolyLine(QVector<QPoint>(object_draw), color);
    qDebug() << __FUNCTION__ << "Sale";
}

