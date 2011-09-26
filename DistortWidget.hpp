#ifndef DISTORTWIDGET_H
#define DISTORTWIDGET_H

#include <MultiWidgets/ImageWidget.hpp>
#include <Radiant/Grid.hpp>
#include <Luminous/Utils.hpp>
#include <Box2D/Box2D.h>

#include "ReachingWidget.hpp"

class DistortWidget : public ReachingWidget {

public:

  DistortWidget(MultiWidgets::Widget * parent = 0);

  void ensureWidgetsHaveBodies();
  void applyForceToBodies(float dt);
  void input(MultiWidgets::GrabManager & gm, float dt);
  void render(Luminous::RenderContext & r);
  void addMovingAndStaticWidgetPair(MultiWidgets::Widget* staticWidget, MultiWidgets::Widget* movingWidget);

};

#endif // DISTORTWIDGET_H
