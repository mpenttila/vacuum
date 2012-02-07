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

linux-* {
LIBS += -lMultiWidgets -lRadiant -lLuminous -lPoetic -lValuable -lFluffy -lResonant -lNimble -lMultiStateDisplay -lVideoDisplay -lMultiTouch -lScreenplay -lPatterns -lBox2D
LIBS += -lSDLmain
LIBS += -lThreadedRendering 
}
macx {
LIBS += -framework MultiWidgets -framework Radiant -framework Luminous -framework Poetic -framework Valuable 
LIBS += -framework Fluffy
LIBS += -framework Resonant -framework Nimble -framework MultiStateDisplay -framework VideoDisplay
LIBS += -framework MultiTouch -framework Screenplay -framework Patterns
LIBS += -framework ThreadedRendering -framework Cocoa -framework OpenGL
LIBS += -lSDL -lSDLmain -lBox2D
}

HEADERS += DistortWidget.hpp widgetlist.hpp VacuumWidget.hpp wordreader.hpp RoundTextBox.hpp logger.hpp ReachingWidget.hpp VacuumReachingWidget.hpp

TARGET = vacuum

