QT += core gui widgets

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += SCAM_LIB_EXPORT

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dummycam.cpp \
    ffmpeg.cpp \
    idscam.cpp \
    scamlib.cpp \
    scam.cpp \
    scamman.cpp \
    basler.cpp \
    imgelement.cpp \
    imgproc.cpp \
    util/colorspaces.cpp \
    v4l2cam.cpp

HEADERS += \
    dummycam.h \
    ffmpeg.h \
    idscam.h \
    scamlib.h \
    scam.h \
    scamman.h \
    basler.h \
    imgelement.h \
    imgproc.h \
    util/colorspaces.h \
    v4l2cam.h

INCLUDEPATH += /usr/lib64
INCLUDEPATH += /usr/local/include/opencv4
INCLUDEPATH += /usr/include/opencv4
INCLUDEPATH += /usr/include/ffmpeg
INCLUDEPATH += /usr/local/lib
INCLUDEPATH += /opt/pylon/include

LIBS += -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_features2d -lueye_api -ludev -lavcodec -lavutil -lavformat -lswscale

# Basler
# env variable PYLON_ROOT must be set
LIBS += $$system("/opt/pylon/bin/pylon-config --libs")
QMAKE_RPATHDIR += -Wl,-rpath,$$system("/opt/pylon/bin/pylon-config --libdir")

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target

DESTDIR = $$_PRO_FILE_PWD_/bin
