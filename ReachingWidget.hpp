#ifndef REACHINGWIDGET_H
#define REACHINGWIDGET_H

#include <MultiWidgets/ImageWidget.hpp>
#include <Radiant/Grid.hpp>
#include <Luminous/Utils.hpp>
#include <Box2D/Box2D.h>
#include <VacuumWidget.hpp>

#include <map>

struct Particle {
	float age;
	float max_age;
	float brightness;

	Nimble::Vector2 pos;

	bool dead() { return age > max_age; }
	Particle(float brightness=1, float max_age=2, Nimble::Vector2 pos=Nimble::Vector2(0,0)) :
			age(0),
			max_age(max_age),
			brightness(brightness),
			pos(pos)
	{
	}
};

namespace {
	float BOX2D_SCALE = 50.0f;
}
class ReachingWidget : public MultiWidgets::Widget {
	// ensure appropriate scale for box2d
protected:
	static Nimble::Vector2 fromBox2D(const b2Vec2 & v) { return Nimble::Vector2(v.x, v.y)*BOX2D_SCALE; }
	static b2Vec2 toBox2D(const Nimble::Vector2 & v) { return b2Vec2(v.x/BOX2D_SCALE, v.y/BOX2D_SCALE); }
	static float toBox2D(float v) { return v/BOX2D_SCALE; }
	
	bool isLeftOfLine(Nimble::Vector2 a, Nimble::Vector2 b, Nimble::Vector2 c);
	
public:
	enum FeatureFlags {
		FEATURE_ARROWS = 1 << 0,
		FEATURE_PARTICLES = 1 << 1,
		FEATURE_VELOCITY_FIELD = 1 << 2,
		FEATURE_GRAVITY = 1 << 3
	};

	ReachingWidget(MultiWidgets::Widget * parent = 0);

	virtual void deleteChild(Widget *w);
	void setFeatureFlags(uint32_t flags);
	void resize(int width, int height);
	void update(float dt);
	void updateParticles(float dt);
	void decayVectorField(Radiant::MemGrid32f (&field)[2], float dt);
	virtual void ensureWidgetsHaveBodies() = 0;
	void ensureGroundInitialized();
	virtual void applyForceToBodies(float dt) = 0;
	void updateBodiesToWidgets();
	Nimble::Vector2 vectorValue(Nimble::Vector2 v);
	Nimble::Vector2 vectorOffset(Nimble::Vector2 pos);
	void blur(float ratio);
	Nimble::Vector2 mapInput(Nimble::Vector2 v);
	MultiWidgets::Widget * findChildInside(Luminous::Transformer & tr, Nimble::Vector2f loc, MultiWidgets::Widget * parent);
	
	void resetVectorField();
	
	virtual void input(MultiWidgets::GrabManager & gm, float dt) = 0;
	virtual void render(Luminous::RenderContext & r) = 0;
	
	virtual void addMovingAndStaticWidgetPair(MultiWidgets::Widget* staticWidget, MultiWidgets::Widget* movingWidget) = 0;

	int w, h;
	Radiant::MemGrid32f m_vectorFields[2];

	struct GLData;

	std::vector<Particle> m_particles;
	std::vector<int> m_free_particles;
	std::set<long> m_currentFingerIds;
	Valuable::ValueIntT<uint32_t> m_featureFlags;
	b2World m_world;
	std::map<void*, b2Body*> m_bodies;

	Valuable::ValueFloat m_bg_alpha;
	Valuable::ValueFloat m_idle_timeout;
	Valuable::ValueFloat m_idle_ok;
	Valuable::ValueFloat m_input_pull_intensity;
	Valuable::ValueFloat m_input_pull_angle;
	Valuable::ValueInt m_input_pull_type;
	Valuable::ValueVector2f m_gravity;
	Valuable::ValueFloat m_tubeWidth;
	Valuable::ValueFloat m_decayPerSec;
	Valuable::ValueFloat m_blurFactor;;

	float m_particleAcc;
	float m_blurAcc;

	b2Body * groundBody;
};

struct ReachingWidget::GLData : public Luminous::GLResource {
	GLData(Luminous::GLResources* res) :
		Luminous::GLResource(res),
		m_shader(0),
		m_vbo(0),
		m_fbo(0)
		//m_particleShader(0)
	{
		m_shader = Luminous::GLSLProgramObject::fromFiles(0, "distort.frag");
		//m_particleShader = Luminous::GLSLProgramObject::fromFiles("particles.vert", "particles.frag");
		m_fbo = new Luminous::Framebuffer();
		m_vbo = new Luminous::VertexBuffer();
	}
	virtual ~GLData()
	{
		delete m_shader;
		//delete m_particleShader;
		delete m_fbo;
	}

	Luminous::GLSLProgramObject* m_shader;
	Luminous::VertexBuffer * m_vbo;
	Luminous::Framebuffer * m_fbo;
	Luminous::GLSLProgramObject * m_particleShader;
	// x, y & read buffer
	Luminous::ContextVariableT<Luminous::Texture2D> m_tex[3];
	Luminous::ContextVariableT<Luminous::Texture2D> m_particleTexture;
};

#endif // REACHINGWIDGET_H
