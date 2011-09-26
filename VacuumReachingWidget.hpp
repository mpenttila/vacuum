#ifndef VacuumReachingWidget_H
#define VacuumReachingWidget_H

#include <MultiWidgets/ImageWidget.hpp>
#include <Radiant/Grid.hpp>
#include <Luminous/Utils.hpp>
#include <Box2D/Box2D.h>
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
	
private:

	void addVacuumWidget(long fingerId, Nimble::Vector2 center);
	void deleteVacuumWidget(long fingerId);
	void modifyVacuumWidget(long fingerId, double rotation, float intensity);
	std::map<long, VacuumWidget*> m_vacuumWidgets;
	std::map<void*, MultiWidgets::Widget *> m_static_to_moving;
	std::map<void*, MultiWidgets::Widget *> m_moving_to_static;

};

#endif // VacuumReachingWidget_H
