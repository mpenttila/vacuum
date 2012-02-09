#ifndef VacuumReachingWidget_H
#define VacuumReachingWidget_H

#include <MultiWidgets/ImageWidget.hpp>
#include <Radiant/Grid.hpp>
#include <Luminous/Utils.hpp>
#include <Box2D/Box2D.h>
#include <Radiant/Mutex.hpp>
#include <map>

#include "ReachingWidget.hpp"

class VacuumReachingWidget : public ReachingWidget {

public:

	VacuumReachingWidget(MultiWidgets::Widget * parent = 0);

	void ensureWidgetsHaveBodies();
	void applyForceToBodies(float dt);
	void input(MultiWidgets::GrabManager & gm, float dt);
	void render(Luminous::RenderContext & r);
	void addMovingAndStaticWidgetPair(MultiWidgets::Widget* staticWidget, MultiWidgets::Widget* movingWidget);
    void update(float dt);
    void decayVectorField(Radiant::MemGrid32f (&field)[2], float dt);
    virtual void processMessage(const char* msg, Radiant::BinaryData& bd);
    void resetAndClear();
	
private:

    void addVacuumWidget(long fingerId, Nimble::Vector2 center, double rotation);
	void deleteVacuumWidget(long fingerId);
	void modifyVacuumWidget(long fingerId, double rotation, float intensity);
    void applyVacuum();
	std::map<long, VacuumWidget*> m_vacuumWidgets;
	std::map<void*, MultiWidgets::Widget *> m_static_to_moving;
	std::map<void*, MultiWidgets::Widget *> m_moving_to_static;
    std::map<MultiWidgets::Widget *, MultiWidgets::Widget *> m_vacuumClones;

    Radiant::Mutex cloneMapMutex;

};

#endif // VacuumReachingWidget_H
