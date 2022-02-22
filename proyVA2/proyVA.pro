#-------------------------------------------------
#
# Project created by QtCreator 2022-01-23T13:03:32
#
#-------------------------------------------------

QT += core gui opengl

TARGET = proyVA
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    imgviewer.cpp 

HEADERS  += mainwindow.h \
    imgviewer.h 

INCLUDEPATH += /usr/include/opencv4 /usr/local/include/opencv4
LIBS += -L/usr/lib -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs -lopencv_core -lopencv_highgui -lopencv_features2d -lopencv_flann -lopencv_video -lopencv_calib3d

FORMS += mainwindow.ui \
    operOrderForm.ui \
    pixelTForm.ui \
    lFilterForm.ui
