#ifndef ROUNDTEXTBOX_HPP
#define ROUNDTEXTBOX_HPP

class RoundTextBox : public virtual MultiWidgets::TextBox
{
	public:
		RoundTextBox(MultiWidgets::Widget * parent, const char * text, uint32_t a);
		virtual ~RoundTextBox();

		//virtual void render(Luminous::RenderContext & r);

		void cachedRender(Luminous::RenderContext & r);

};

#endif
