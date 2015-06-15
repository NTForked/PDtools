TEMPLATE  = app
TARGET 	  = peridyn
CONFIG   += console
CONFIG   -= app_bundle
CONFIG   -= qt

include(../../../defaults.pri)

INCLUDEPATH += $$SRC_DIR/PDtools
LIBS += -lconfig++
LIBS += $$TOP_OUT_PWD/src/PDtools/libPDtools.a

# For dynamic linking:
#LIBS +=  -L$$TOP_OUT_PWD/src/PDtools -lPDtools
#QMAKE_LFLAGS += -Wl,--rpath=$$TOP_OUT_PWD/src/PDtools

message(from peridyn.pro $$LIBS)

SOURCES += \
    main.cpp \
    pdsolver.cpp

HEADERS += \
    pdsolver.h
