CONFIG += release
CONFIG -= qt
CONFIG -= app_bundle

SOURCES += Main.cpp \
    DistortWidget.cpp \
    widgetlist.cpp \
    VacuumWidget.cpp \
    wordreader.cpp \
    RoundTextBox.cpp \
    logger.cpp \
	ReachingWidget.cpp \
	VacuumReachingWidget.cpp

exists(/home/loop/multitouch/cornerstone) {
	INCLUDEPATH += /home/loop/multitouch/cornerstone/ /home/loop/multitouch/cornerstone/multitude
	QMAKE_LFLAGS += -L/home/loop/multitouch/cornerstone/lib -L/home/loop/multitouch/cornerstone/multitude/lib
}

LIBS += -lMultiWidgets -lRadiant -lLuminous -lPoetic -lValuable -lFluffy -lResonant -lNimble -lMultiStateDisplay -lVideoDisplay -lMultiTouch -lScreenplay -lPatterns -lBox2D
LIBS += -lSDLmain
linux-* { LIBS += -lThreadedRendering }

HEADERS += DistortWidget.hpp widgetlist.hpp VacuumWidget.hpp wordreader.hpp RoundTextBox.hpp logger.hpp ReachingWidget.hpp VacuumReachingWidget.hpp

TARGET = vacuum

