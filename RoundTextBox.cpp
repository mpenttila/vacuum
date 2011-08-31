
RoundTextBox::RoundTextBox(MultiWidgets::Widget * parent, const char * text, uint32_t a) : TextBox(parent, text, a)
{
}

RoundTextBox::~RoundTextBox()
{
}

void RoundTextBox::cachedRender(Luminous::RenderContext & r)
{
	Luminous::FramebufferResource & fbr = m_fbResource.ref(r.resources());

	fbr.texture().bind();

	// Render cached
	glEnable(GL_TEXTURE_2D);
	Luminous::Utils::glUsualBlend();

	const float pw = m_cssPaddingWidth.asFloat();
	const float bw = m_cssBorderWidth.asFloat();

	r.pushTransformRightMul(Nimble::Matrix3::translate2D(Nimble::Vector2(-pw - bw, -pw - bw)));

	float color[] = {1.f, 1.f, 1.f, 1.f};
	r.drawTexRect(size() + 2.f * Nimble::Vector2(pw + bw, pw + bw), color);

	r.popTransform();

	glDisable(GL_TEXTURE_2D);
}
