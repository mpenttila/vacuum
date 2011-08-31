
#ifndef VACUUMWIDGET_HPP
#define VACUUMWIDGET_HPP

#include <MultiWidgets/Widget.hpp>

class VacuumWidget : public virtual MultiWidgets::Widget
{
	public:
	VacuumWidget(MultiWidgets::Widget * parent = 0);
	virtual ~VacuumWidget();

	/// Renders the widget, with OpenGL
	virtual void render(Luminous::RenderContext & r);
	/// Tests if the given point is inside this widget
	//virtual bool isInside(Nimble::Vector2 v) const;

	virtual const char * type() const;

	inline void setThickness(float thickness)
	{ m_thickness = thickness; }

	inline void setArc(float percentage)
	{ m_arc = percentage; }

	private:
	float m_thickness;
        float m_arc;
	// We draw arcs inside the donuts with this color:
	Radiant::Color m_arcColor;
};


#endif
