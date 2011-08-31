CONFIG += release
CONFIG -= qt
CONFIG -= app_bundle

SOURCES += Main.cpp \
    distortwidget.cpp \
    widgetlist.cpp \
    VacuumWidget.cpp \
    wordreader.cpp \
    RoundTextBox.cpp

exists(/home/loop/multitouch/cornerstone) {
	INCLUDEPATH += /home/loop/multitouch/cornerstone/ /home/loop/multitouch/cornerstone/multitude
	QMAKE_LFLAGS += -L/home/loop/multitouch/cornerstone/lib -L/home/loop/multitouch/cornerstone/multitude/lib
}

LIBS += -lMultiWidgets -lRadiant -lLuminous -lPoetic -lValuable -lFluffy -lResonant -lNimble -lMultiStateDisplay -lVideoDisplay -lMultiTouch -lScreenplay -lPatterns -lBox2D
LIBS += -lSDLmain
linux-* { LIBS += -lThreadedRendering }

HEADERS += distortwidget.hpp widgetlist.hpp RotatorOperator.hpp VacuumWidget.hpp wordreader.hpp RoundTextBox.hpp

TARGET = vacuum

