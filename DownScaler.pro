QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = DownScaler

VERSION = 2.0
QMAKE_TARGET_COMPANY = Varius Lab
QMAKE_TARGET_PRODUCT = DownScaler
QMAKE_TARGET_DESCRIPTION = Downscale and compress images in batches. Supported formats: JPEG, JPG, PNG, BMP, TIFF.
QMAKE_TARGET_COPYRIGHT = Varvara Petrova 2023
RC_ICONS += icon.ico

SOURCES += \
    main.cpp \
    MainWindow.cpp\
    FileExistsDialog.cpp


HEADERS += \
    MainWindow.h \
    FileExistsDialog.h


FORMS += \
    MainWindow.ui \
    FileExistsDialog.ui


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc
