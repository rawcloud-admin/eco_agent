TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += main.cpp \
    proc_parser.cpp \
    system.cpp \
    log.cpp

HEADERS += \
    proc_parser.h \
    system.h \
    log.h

LIBS += -lpthread

OTHER_FILES += \
    Makefile.sh4

