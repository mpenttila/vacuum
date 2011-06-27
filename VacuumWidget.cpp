#include "VacuumWidget.hpp"

#include <Luminous/Utils.hpp>

  VacuumWidget::VacuumWidget(MultiWidgets::Widget * parent)
    : Widget(parent),
      m_thickness(25),
      m_arcColor("#000000") // Solid black
  {}

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
    /* The outer radius of the donut. */
    float outerRadius = width() * 0.5f;

    /* Calculate the center of  the donut. */
    Nimble::Vector3 center = m * Nimble::Vector3(outerRadius, outerRadius, 1);

    /* The radius for rendering. The circle function uses the center
       of the line as the radius, while we use outer edge. */
    float radius = scale * (outerRadius - m_thickness * 0.5f);

    /* How many segments should we use for the circle. */
    int segments = Nimble::Math::Max(scale * outerRadius * 0.7f, 30.0f);

    /* Enable the typical OpenGL blending mode. The blending is
       necessary, since the functions that do silhuette anti-aliasing
       (i.e. the circle function below) */
    Luminous::Utils::glUsualBlend();

    /* This functions draws the main circle, with anti-aliased edges. */
    Luminous::Utils::
      glFilledSoftCircle(center.data(), radius,
             m_thickness * scale, 1.0f,
             segments,
             color().data());

    /* Lets draw an arc inside the donut. This adds to the graphical
       feeling and without it users would not see when they rotate the
       widget. */
    Luminous::Utils::
      glFilledSoftArc(center.data(), radius,
              m_rotation, m_rotation + 5.0f,
              scale, 1.0f,
              segments,
              m_arcColor.data());

    r.popTransform();
  }

  /* The isInside needs to be overridden since this widget is not
     rectangular. The incoming vector is already in the local
     coordinate system. */
  bool VacuumWidget::isInside(Nimble::Vector2 v) const
  {
    /* Calculate the outer radius of the circle. */
    float outerRadius = width() * 0.5f;

    /* Calculate the center of circle.*/
    Nimble::Vector2 center(outerRadius, outerRadius);
    /* Compute how far the vector is from the center of the donut. */
    float distance = (v - center).length();

    /* If the distance exceeds the outer radius, or if the distance is
       less than the inner radius, then point v is not inside this
       widget. */
    if(distance > outerRadius || distance < (outerRadius - m_thickness))
      return false;

    return true;
  }
