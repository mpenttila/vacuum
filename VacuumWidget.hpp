
#ifndef VacuumWidget_DONUT_WIDGET_HPP
#define VacuumWidget_DONUT_WIDGET_HPP

#include <MultiWidgets/Widget.hpp>

/** This is a donut-shaped widget. It demonstrates the use of virtual
  functions <b>isInside</b> and <b>render</b>.
  
*/
class VacuumWidget : public virtual MultiWidgets::Widget
{
	public:
	VacuumWidget(MultiWidgets::Widget * parent = 0);
	virtual ~VacuumWidget();

	/// Renders the widget, with OpenGL
	virtual void render(Luminous::RenderContext & r);
	/// Tests if the given point is inside this widget
	virtual bool isInside(Nimble::Vector2 v) const;

	inline void setThickness(float thickness)
	{ m_thickness = thickness; }

	private:
	float m_thickness;
	// We draw arcs inside the donuts with this color:
	Radiant::Color m_arcColor;
};


#endif
