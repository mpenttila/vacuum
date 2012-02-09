#include "VacuumWidget.hpp"

#include <Luminous/Utils.hpp>

#include <iostream>

  VacuumWidget::VacuumWidget(MultiWidgets::Widget * parent)
    : Widget(parent),
      m_thickness(10),
      m_arc(0.1),
      m_arcColor("#ffffffdd")
  {
       // setSize(Nimble::Vector2(200, 200));
      //  setColor(Nimble::Vector4(0.4, 1.0f - 0.2, 1.0, 0.97));
	//setInputTransparent(true);
  }

  VacuumWidget::~VacuumWidget()
  {}

  void VacuumWidget::renderContent(Luminous::RenderContext &r){

      // The outer radius of the widget.
      float outerRadius = width() * 0.5f;

      // The radius for rendering. The circle function uses the center
      // of the line as the radius, while we use outer edge.
      float radius = m_scale * (outerRadius - m_thickness * 0.5f);
      float radius2 = m_scale * (outerRadius - m_thickness * 8.0f);
      float radius3 = m_scale * (outerRadius - m_thickness * 3.0f);

      // How many segments should we use for the circle.
      int segments = Nimble::Math::Max(m_scale * outerRadius * 0.7f, 30.0f);

      r.drawArc(0.5f*size(), radius, 0, 2*Nimble::Math::PI, m_thickness * m_scale, 2, color().data(), segments);
      r.drawArc(0.5f*size(), radius2, 0, 2*Nimble::Math::PI, m_thickness * m_scale, 2, color().data(), segments);

      // Calculate end points
      float start = 0 - m_arc * Nimble::Math::PI;
      float end = m_arc * Nimble::Math::PI;

      r.drawArc(0.5f*size(), radius3, start, end, m_thickness * 3 * m_scale, 7.0f, m_arcColor.data(), segments);

  }

  const char * VacuumWidget::type() const
  {
	return "VacuumWidget";
  }




