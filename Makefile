#############################################################################
# Makefile for building: vacuum
# Generated by qmake (1.07a) (Qt 3.3.8b) on: Tue Jun 28 13:39:58 2011
# Project:  distort.pro
# Template: app
# Command: $(QMAKE) -o Makefile distort.pro
#############################################################################

####### Compiler, tools and options

CC       = gcc
CXX      = g++
LEX      = flex
YACC     = yacc
CFLAGS   = -pipe -g -Wall -W -O2 -D_REENTRANT  
CXXFLAGS = -pipe -g -Wall -W -O2 -D_REENTRANT  
LEXFLAGS = 
YACCFLAGS= -d
INCPATH  = -I/usr/share/qt3/mkspecs/default -I.
LINK     = g++
LFLAGS   = 
LIBS     = $(SUBLIBS)  -lMultiWidgets -lRadiant -lLuminous -lPoetic -lValuable -lFluffy -lResonant -lNimble -lMultiStateDisplay -lVideoDisplay -lMultiTouch -lScreenplay -lPatterns -lBox2D -lSDLmain -lThreadedRendering -lpthread
AR       = ar cqs
RANLIB   = 
MOC      = /usr/share/qt3/bin/moc
UIC      = /usr/share/qt3/bin/uic
QMAKE    = qmake
TAR      = tar -cf
GZIP     = gzip -9f
COPY     = cp -f
COPY_FILE= $(COPY)
COPY_DIR = $(COPY) -r
INSTALL_FILE= $(COPY_FILE)
INSTALL_DIR = $(COPY_DIR)
DEL_FILE = rm -f
SYMLINK  = ln -sf
DEL_DIR  = rmdir
MOVE     = mv -f
CHK_DIR_EXISTS= test -d
MKDIR    = mkdir -p

####### Output directory

OBJECTS_DIR = ./

####### Files

HEADERS = distortwidget.h \
		widgetlist.h \
		RotatorOperator.h \
		VacuumWidget.hpp
SOURCES = Main.cpp \
		distortwidget.cpp \
		widgetlist.cpp \
		VacuumWidget.cpp
OBJECTS = Main.o \
		distortwidget.o \
		widgetlist.o \
		VacuumWidget.o
FORMS = 
UICDECLS = 
UICIMPLS = 
SRCMOC   = 
OBJMOC = 
DIST	   = distort.pro
QMAKE_TARGET = vacuum
DESTDIR  = 
TARGET   = vacuum

first: all
####### Implicit rules

.SUFFIXES: .c .o .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(UICDECLS) $(OBJECTS) $(OBJMOC)  
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJMOC) $(OBJCOMP) $(LIBS)

mocables: $(SRCMOC)
uicables: $(UICDECLS) $(UICIMPLS)

$(MOC): 
	( cd $(QTDIR)/src/moc && $(MAKE) )

Makefile: distort.pro  /usr/share/qt3/mkspecs/default/qmake.conf 
	$(QMAKE) -o Makefile distort.pro
qmake: 
	@$(QMAKE) -o Makefile distort.pro

dist: 
	@mkdir -p .tmp/vacuum && $(COPY_FILE) --parents $(SOURCES) $(HEADERS) $(FORMS) $(DIST) .tmp/vacuum/ && ( cd `dirname .tmp/vacuum` && $(TAR) vacuum.tar vacuum && $(GZIP) vacuum.tar ) && $(MOVE) `dirname .tmp/vacuum`/vacuum.tar.gz . && $(DEL_FILE) -r .tmp/vacuum

mocclean:
uiclean:

yaccclean:
lexclean:
clean:
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core


####### Sub-libraries

distclean: clean
	-$(DEL_FILE) $(TARGET) $(TARGET)


FORCE:

####### Compile

Main.o: Main.cpp RotatorOperator.h \
		distortwidget.h \
		widgetlist.h \
		VacuumWidget.hpp \
		Box2D/Box2D.h \
		Box2D/Common/b2Settings.h \
		Box2D/Collision/Shapes/b2CircleShape.h \
		Box2D/Collision/Shapes/b2PolygonShape.h \
		Box2D/Collision/b2BroadPhase.h \
		Box2D/Collision/b2Distance.h \
		Box2D/Collision/b2DynamicTree.h \
		Box2D/Collision/b2TimeOfImpact.h \
		Box2D/Dynamics/b2Body.h \
		Box2D/Dynamics/b2Fixture.h \
		Box2D/Dynamics/b2WorldCallbacks.h \
		Box2D/Dynamics/b2TimeStep.h \
		Box2D/Dynamics/b2World.h \
		Box2D/Dynamics/Contacts/b2Contact.h \
		Box2D/Dynamics/Joints/b2DistanceJoint.h \
		Box2D/Dynamics/Joints/b2FrictionJoint.h \
		Box2D/Dynamics/Joints/b2GearJoint.h \
		Box2D/Dynamics/Joints/b2LineJoint.h \
		Box2D/Dynamics/Joints/b2MouseJoint.h \
		Box2D/Dynamics/Joints/b2PrismaticJoint.h \
		Box2D/Dynamics/Joints/b2PulleyJoint.h \
		Box2D/Dynamics/Joints/b2RevoluteJoint.h \
		Box2D/Dynamics/Joints/b2WeldJoint.h \
		Box2D/Collision/Shapes/b2Shape.h \
		Box2D/Common/b2BlockAllocator.h \
		Box2D/Common/b2Math.h \
		Box2D/Collision/b2Collision.h \
		Box2D/Common/b2StackAllocator.h \
		Box2D/Dynamics/b2ContactManager.h \
		Box2D/Dynamics/Joints/b2Joint.h

distortwidget.o: distortwidget.cpp distortwidget.h \
		Box2D/Box2D.h \
		Box2D/Common/b2Settings.h \
		Box2D/Collision/Shapes/b2CircleShape.h \
		Box2D/Collision/Shapes/b2PolygonShape.h \
		Box2D/Collision/b2BroadPhase.h \
		Box2D/Collision/b2Distance.h \
		Box2D/Collision/b2DynamicTree.h \
		Box2D/Collision/b2TimeOfImpact.h \
		Box2D/Dynamics/b2Body.h \
		Box2D/Dynamics/b2Fixture.h \
		Box2D/Dynamics/b2WorldCallbacks.h \
		Box2D/Dynamics/b2TimeStep.h \
		Box2D/Dynamics/b2World.h \
		Box2D/Dynamics/Contacts/b2Contact.h \
		Box2D/Dynamics/Joints/b2DistanceJoint.h \
		Box2D/Dynamics/Joints/b2FrictionJoint.h \
		Box2D/Dynamics/Joints/b2GearJoint.h \
		Box2D/Dynamics/Joints/b2LineJoint.h \
		Box2D/Dynamics/Joints/b2MouseJoint.h \
		Box2D/Dynamics/Joints/b2PrismaticJoint.h \
		Box2D/Dynamics/Joints/b2PulleyJoint.h \
		Box2D/Dynamics/Joints/b2RevoluteJoint.h \
		Box2D/Dynamics/Joints/b2WeldJoint.h \
		Box2D/Collision/Shapes/b2Shape.h \
		Box2D/Common/b2BlockAllocator.h \
		Box2D/Common/b2Math.h \
		Box2D/Collision/b2Collision.h \
		Box2D/Common/b2StackAllocator.h \
		Box2D/Dynamics/b2ContactManager.h \
		Box2D/Dynamics/Joints/b2Joint.h

widgetlist.o: widgetlist.cpp widgetlist.h

VacuumWidget.o: VacuumWidget.cpp VacuumWidget.hpp

####### Install

install:  

uninstall:  

