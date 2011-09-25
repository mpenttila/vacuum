#include "VacuumWidget.hpp"

#include <Luminous/Utils.hpp>

#include <iostream>

  VacuumWidget::VacuumWidget(MultiWidgets::Widget * parent)
    : Widget(parent),
      m_thickness(25),
      m_arc(0.1),
      m_arcColor("#ffffffdd")
  {
       // setSize(Nimble::Vector2(200, 200));
      //  setColor(Nimble::Vector4(0.4, 1.0f - 0.2, 1.0, 0.97));
	//setInputTransparent(true);
  }

  VacuumWidget::~VacuumWidget()
  {}

  void VacuumWidget::render(Luminous::RenderContext & r)
  {
    /* Apply the current transformation matrix to the current
       rendering matrix. */
    r.pushTransformRightMul(transform());

    /* Get the current rendering transform, starting from the root. */
    Nimble::Matrix3 m = r.transform();

    float scale = m.extractScale();
    /* The outer radius of the widget. */
    float outerRadius = width() * 0.5f;

    /* Calculate the center of the widget. */
    Nimble::Vector3 center = m * Nimble::Vector3(outerRadius, outerRadius, 1);

    /* The radius for rendering. The circle function uses the center
       of the line as the radius, while we use outer edge. */
    float radius = scale * (outerRadius - m_thickness * 0.5f);
    float radius2 = scale * (outerRadius - m_thickness * 4.0f);
    float radius3 = scale * (outerRadius - m_thickness * 2.0f);

    /* How many segments should we use for the circle. */
    int segments = Nimble::Math::Max(scale * outerRadius * 0.7f, 30.0f);

    /* Enable the typical OpenGL blending mode. */
    Luminous::Utils::glUsualBlend();

    /* Draw two fixed circles */
    Luminous::Utils::
      glFilledSoftCircle(center.data(), radius,
             m_thickness * scale, 1.0f,
             segments,
             color().data());

    Luminous::Utils::
      glFilledSoftCircle(center.data(), radius2,
             m_thickness * scale, 1.0f,
             segments,
             color().data());

    // Calculate end points
    float start = Nimble::Math::HALF_PI + m_rotation - (m_arc * Nimble::Math::PI);
    float end = Nimble::Math::HALF_PI + m_rotation + (m_arc * Nimble::Math::PI);

    /* Draw the arc representing the influence area */
    Luminous::Utils::
      glFilledSoftArc(center.data(), radius3,
              start, end,
              m_thickness * 2 * scale, 7.0f,
              segments,
              m_arcColor.data());

    r.popTransform();
  }

  const char * VacuumWidget::type() const
  {
	return "VacuumWidget";
  }




