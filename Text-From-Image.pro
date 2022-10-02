QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17


#pkg-config --libs tesseract
#pkg-config --libs opencv4
unix{
CONFIG += link_pkgconfig

PKGCONFIG +=  opencv4
PKGCONFIG +=  tesseract
}

SOURCES = \
    ./src/main.cpp \
    ./src/mainwindow.cpp \
    ./src/imageframe.cpp \
    ./src/imagetextobject.cpp \
    ./src/options.cpp \
    ./src/colortray.cpp \
    ./src/tabscroll.cpp

HEADERS = \
    ./headers/mainwindow.h \
    ./headers/imageframe.h \
    ./headers/imagetextobject.h \
    ./headers/options.h \
    ./headers/colortray.h \
    ./headers/tabscroll.h

FORMS = \
    ./forms/mainwindow.ui \
    #./forms/_mainwindow.ui \
    ./forms/imagetextobject.ui \
    ./forms/options.ui \
    ./forms/colortray.ui \
    ./forms/tabscroll.ui

RESOURCES += \
    ./res/res.qrc \

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
	res/style.qss


