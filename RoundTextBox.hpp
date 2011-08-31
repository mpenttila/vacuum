#ifndef ROUNDTEXTBOX_HPP
#define ROUNDTEXTBOX_HPP

class RoundTextBox : public virtual MultiWidgets::TextBox
{
	public:
		RoundTextBox(MultiWidgets::Widget * parent, const char * text, uint32_t a);
		virtual ~RoundTextBox();
		
		RoundTextBox * clone();

		//virtual void render(Luminous::RenderContext & r);

		virtual void cachedRender(Luminous::RenderContext & r);
		void renderContent(Luminous::RenderContext & r);
		virtual bool isInside(Nimble::Vector2 v) const;
		
		inline void setType(std::string type)
		{ m_type = type; }
	
		virtual const char * type() const;

	private:
		std::string m_type;

};

#endif
