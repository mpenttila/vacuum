#ifndef ROUNDTEXTBOX_HPP
#define ROUNDTEXTBOX_HPP

#include <MultiWidgets/TextBox.hpp>

class RoundTextBox : public virtual MultiWidgets::TextBox
{
	public:
		RoundTextBox(MultiWidgets::Widget * parent, const char * text, uint32_t a);
		virtual ~RoundTextBox();
		
		RoundTextBox * clone();

		//virtual void render(Luminous::RenderContext & r);

        //virtual void cachedRender(Luminous::RenderContext & r);
		void renderContent(Luminous::RenderContext & r);
        void renderBorder(Luminous::RenderContext &r);
		virtual bool isInside(Nimble::Vector2 v) const;
		
		inline void setType(std::string type)
		{ m_type = type; }
	
        virtual const char * type() const;

        void setPlayer(int i);

        int player() const;

	private:
        std::string m_type;
        int m_playernumber;
};

#endif
