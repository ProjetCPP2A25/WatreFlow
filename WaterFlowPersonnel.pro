QT += core gui widgets
CONFIG += c++17

SOURCES += main.cpp personnel.cpp barchart.cpp
HEADERS += personnel.h barchart.h
FORMS   += personnel.ui
SOURCES += \
    main.cpp \
    personnel.cpp \
    barchart.cpp \
    livraisons.cpp

HEADERS += \
    personnel.h \
    barchart.h \
    livraisons.h

FORMS += \
    personnel.ui \
    livraisons.ui