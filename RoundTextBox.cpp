#include <MultiWidgets/TextBox.hpp>
#include <Poetic/GPUWrapperFont.hpp>
#include <Poetic/Utils.hpp>
#include <Luminous/GLContext.hpp>
#include <Luminous/Utils.hpp>
#include <Luminous/RenderContext.hpp>
#include <Luminous/Error.hpp>
#include <Radiant/FileUtils.hpp>
#include <MultiWidgets/Export.hpp>
#include <MultiWidgets/Widget.hpp>
#include <Poetic/FontManager.hpp>
#include <Valuable/ValueString.hpp>
#include <Valuable/ValueInt.hpp>

#include <Nimble/Matrix3.hpp>
#include <Nimble/Vector2.hpp>

#include <iostream>

#include "RoundTextBox.hpp"

using namespace Radiant;
using namespace Nimble;

RoundTextBox::RoundTextBox(MultiWidgets::Widget * parent, const char * text, uint32_t a) : TextBox(parent, text, a), m_type("RoundTextBox")
{
}

RoundTextBox::~RoundTextBox()
{
}

RoundTextBox * RoundTextBox::clone(){

	RoundTextBox * tb2 = new RoundTextBox(parent(), 0, MultiWidgets::TextBox::HCENTER);
	tb2->setCSSClass("FloatingWord_clone");
	tb2->setStyle(style());
	tb2->setText(text());
	tb2->setWidth(width());
	tb2->setHeight(height());
	tb2->setAlignFlags(MultiWidgets::TextBox::HCENTER | MultiWidgets::TextBox::VCENTER);
	tb2->setDepth(depth());
	tb2->setLocation(mapToParent(Nimble::Vector2(0,0)));	
	tb2->setScale(scale());
	tb2->setRotation(rotation());
	tb2->setType("clone");
	tb2->setFaceSize(faceSize());
	return tb2;	
}

void RoundTextBox::cachedRender(Luminous::RenderContext & r)
{
	std::cout << "RoundTextBox::cachedRender called" << std::endl;
	Luminous::FramebufferResource & fbr = m_fbResource.ref(r.resources());

	fbr.texture().bind();

	// Render cached
	glEnable(GL_TEXTURE_2D);
	Luminous::Utils::glUsualBlend();

	const float pw = m_cssPaddingWidth.asFloat();
	const float bw = m_cssBorderWidth.asFloat();

	r.pushTransformRightMul(Nimble::Matrix3::translate2D(Nimble::Vector2(-pw - bw, -pw - bw)));

	// Get the current rendering transform, starting from the root.
	Nimble::Matrix3 m = r.transform();

	float scale = m.extractScale();
	// The outer radius of the widget.
	float outerRadius = width() * 0.5f;

	// Calculate the center of the widget.
	Nimble::Vector3 center = m * Nimble::Vector3(outerRadius, outerRadius, 1);

	// The radius for rendering. The circle function uses the center
	//   of the line as the radius, while we use outer edge.
	float radius = scale * (outerRadius);

	// How many segments should we use for the circle.
	int segments = Nimble::Math::Max(scale * outerRadius * 0.7f, 30.0f);

	float color[] = {1.f, 1.f, 1.f, 1.f};
	//r.drawTexRect(size() + 2.f * Nimble::Vector2(pw + bw, pw + bw), color);

	r.drawCircle(center.data(), radius, color, segments);

	r.popTransform();

	glDisable(GL_TEXTURE_2D);
}

void RoundTextBox::renderContent(Luminous::RenderContext & r)
{
	// Now mutex() in SDK 1.2
	// Luminous::GLContext::Guard lock(r.glContext());
	Luminous::GLContext::Guard lock(r.glContext()->mutex());

	// Get the basic box from standard widgets.
	glDisable(GL_TEXTURE_2D);
	//Widget::renderContent(r);
	// Replaced with circle code:
	const Nimble::Matrix3 & m = r.transform();
	Radiant::Color c = color();
	r.useCurrentBlendMode();
	Luminous::GLSLProgramObject * shader = bindShader(r);
	float scale = m.extractScale();
	float outerRadius = width() * 0.5f;
	Nimble::Vector2 center(width()*0.5f, height()*0.5f);
	float radius = scale * (outerRadius);
	int segments = Nimble::Math::Max(scale * outerRadius * 0.7f, 30.0f);
	float color[] = {c.red(),c.green(),c.blue(),c.alpha()};
	r.drawCircle(center, radius, color, segments);
	if(shader) shader->unbind();
	// Circle code ends

	if(flags() & CACHED_RENDER) {

		Luminous::FramebufferResource & fbr = m_fbResource.ref(r.resources());

		// Update cache if needed
		if((int) m_renderCacheGeneration != fbr.generation())
		updateRenderCache(r);

		// Render from cache
		cachedRender(r);
	} else {
		renderText(r);
	}
 }

bool RoundTextBox::isInside(Nimble::Vector2 v) const
{
	/* Calculate the outer radius of the circle. */
	float outerRadius = width() * 0.5f;

	/* Calculate the center of circle.*/
	Nimble::Vector2 center(outerRadius, outerRadius);
	/* Compute how far the vector is from the cebter of the donut. */
	float distance = (v - center).length();

	/* If the distance exceeds the outer radius, then point v is not inside this widget. */

	//std::cout << "distance : " << distance << " outerRadius: " << outerRadius << std::endl;

	if(distance > outerRadius)
		return false;

	return true;
}

const char * RoundTextBox::type() const
{
	return m_type.c_str();
}

