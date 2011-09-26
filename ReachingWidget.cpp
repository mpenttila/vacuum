#include "ReachingWidget.hpp"

#include <Luminous/Utils.hpp>
#include <Luminous/FramebufferObject.hpp>
#include <Nimble/Random.hpp>
#include <Radiant/Color.hpp>

#include <Box2D/Box2D.h>

#define INPUT_PULL_NORMAL 0
#define INPUT_PULL_INVERSELY_PROPORTIONAL 1

/*
struct ReachingWidget::GLData : public Luminous::GLResource {
	GLData(Luminous::GLResources* res) :
		Luminous::GLResource(res),
		m_shader(0),
		m_vbo(0),
		m_fbo(0),
		m_particleShader(0)
	{
		m_shader = Luminous::GLSLProgramObject::fromFiles(0, "distort.frag");
		m_particleShader = Luminous::GLSLProgramObject::fromFiles("particles.vert", "particles.frag");
		m_fbo = new Luminous::Framebuffer();
		m_vbo = new Luminous::VertexBuffer();
	}
	virtual ~GLData()
	{
		delete m_shader;
		delete m_particleShader;
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
*/

ReachingWidget::ReachingWidget(MultiWidgets::Widget * parent) :
	MultiWidgets::Widget(parent),
	m_featureFlags(this, "feature-flags", FEATURE_PARTICLES | FEATURE_VELOCITY_FIELD),
	m_world(b2Vec2(0,0), true),
	m_bg_alpha(this, "background-alpha", 0.3f),
	m_idle_timeout(this, "idle-timeout", 5.0f),
	m_idle_ok(this, "idle-delay", 1.0f),
	m_input_pull_intensity(this, "pull-intensity", 20.0f),
	m_input_pull_angle(this, "pull-angle", Nimble::Math::PI/16),
	m_input_pull_type(this, "pull-type", INPUT_PULL_NORMAL),
	m_gravity(this, "gravity", Nimble::Vector2(0, 100)),
	m_tubeWidth(this, "tube-width", 0.1f),
	m_decayPerSec(this, "decay-per-sec", 2.0f),
	m_blurFactor(this, "blur-factor", 0.7f),
	m_particleAcc(0),
	m_blurAcc(0)
{
	w = h = 0;
	m_decayPerSec = 20.0f;
}

void ReachingWidget::deleteChild(Widget *w)
{
	if (m_bodies.count(w)) {
		m_world.DestroyBody(m_bodies[w]);
		m_bodies.erase(m_bodies.find(w));
	}
	MultiWidgets::Widget::deleteChild(w);
}

void ReachingWidget::setFeatureFlags(uint32_t flags)
{
	m_featureFlags = flags;
}

void ReachingWidget::resize(int width, int height) {
	if (width == w && height == h)
		return;

	w = width;
	h = height;
	for (int i=0; i < 2; ++i)
		m_vectorFields[i].resize(w, h);
	for (int y=0; y < h; ++y) {
		for (int x=0; x < w; ++x) {
			m_vectorFields[0].get(x, y) = 0;
			m_vectorFields[1].get(x, y) = 0;
		}
	}
}

void ReachingWidget::updateParticles(float dt) {
	static const float time_step = 0.75f;
	m_particleAcc += dt;
	Nimble::RandomUniform & rnd = Nimble::RandomUniform::instance();
	while (m_particleAcc >= time_step) {
		m_particleAcc -= time_step;
		static const int division = 1;

		static const int incrY = 1;
		static const int incrX = 1;

		for (int y=0; y < division*h; y += incrY) {
			for (int x=0; x < division*w; x += incrX) {
				float life = 1 + rnd.rand0X(5.0f);
				float brightness = 0.5 + rnd.rand01();
				Nimble::Vector2 v(x, y);

				//v += Nimble::Vector2(rnd.rand01(), rnd.rand01());
				v += rnd.randVecInCircle();

				Nimble::Vector2 pos(float(v.x)/(w-1)/division, float(v.y)/(h-1)/division);
				pos.clampUnit();
				if (m_free_particles.empty()) {
					m_particles.push_back(Particle(brightness, life, pos));
				} else {
					m_particles[m_free_particles.back()] = Particle(brightness, life, pos);
					m_free_particles.pop_back();
				}
			}
		}
	}

	for (unsigned int i=0; i < m_particles.size(); ++i) {
		Particle & p = m_particles[i];
		bool dead = p.dead();
		p.age += dt;
		if (dead != p.dead()) {
			m_free_particles.push_back(i);
		} else {
			p.pos += vectorOffset(p.pos) * dt;
			p.pos.clampUnit();
		}
	}
}

void ReachingWidget::decayVectorField(Radiant::MemGrid32f (&field)[2], float dt) {
	float xmove = (m_decayPerSec * dt)/width();
	float ymove = (m_decayPerSec * dt)/height();
	float move = Nimble::Math::Max(xmove, ymove);
	float * tx = field[0].data();
	float * ty = field[1].data();
	for (int y=0; y < h; ++y) {
		for (int x=0; x < w; ++x) {
			*tx *= (1 - move);
			*ty *= (1 - move);
			tx++;
			ty++;
		}
	}
}

void ReachingWidget::ensureGroundInitialized() {
	static bool groundInit = false;
	const float border = 1.0f;
	if (!groundInit) {
		groundInit = true;
		b2BodyDef groundDef;
		b2Vec2 c = toBox2D(size() * 0.5f);
		groundDef.position.Set(c.x, c.y);
		groundBody = m_world.CreateBody(&groundDef);
		b2PolygonShape groundBox;
		groundBox.SetAsBox(border, c.y + border, b2Vec2(-c.x - border, 0.0f), 0.0f);
		groundBody->CreateFixture(&groundBox, 0.0f);
		groundBox.SetAsBox(c.x, border, b2Vec2(0.0f, c.y + border), 0.0f);
		groundBody->CreateFixture(&groundBox, 0.0f);
		groundBox.SetAsBox(border, c.y + border, b2Vec2(c.x + border, 0.0f), 0.0f);
		groundBody->CreateFixture(&groundBox, 0.0f);
		groundBox.SetAsBox(c.x, border, b2Vec2(0.0f, -c.y - border), 0.0f);
		groundBody->CreateFixture(&groundBox, 0.0f);
	}
}

void ReachingWidget::updateBodiesToWidgets() {
	for (std::map<void*, b2Body*>::iterator it = m_bodies.begin(); it != m_bodies.end(); ++it) {
		b2Vec2 position = it->second->GetPosition();
		float angle = it->second->GetAngle();
		Widget * w = (Widget*)it->first;
		if (w->inputTransparent()) {
			it->second->SetTransform(toBox2D(w->mapToParent(w->size()*0.5f)), w->rotation());
		} else {
			w->setCenterLocation(fromBox2D(position));
			w->setRotation(angle);
		}
	}
}

void ReachingWidget::update(float dt)
{
	MultiWidgets::Widget::update(dt);
	if (m_featureFlags & FEATURE_VELOCITY_FIELD)
		decayVectorField(m_vectorFields, dt);

	if (m_featureFlags & FEATURE_PARTICLES) {
		updateParticles(dt);
	}

	if (m_featureFlags & FEATURE_VELOCITY_FIELD) {
		const float timestep = 0.02f;
		m_blurAcc += dt;
		while (m_blurAcc > timestep) {
			blur(m_blurFactor);
			m_blurAcc -= timestep;
		}
	}

	ensureWidgetsHaveBodies();
	ensureGroundInitialized();

	applyForceToBodies(dt);

	if (m_featureFlags & FEATURE_GRAVITY)
		m_world.SetGravity(b2Vec2(m_gravity.x(), m_gravity.y()));

	for (int i=0; i < 1; ++i) {
		m_world.Step(dt/1, 10, 10);
	}
	m_world.ClearForces();

	updateBodiesToWidgets();
}

Nimble::Vector2 ReachingWidget::vectorValue(Nimble::Vector2 v)
{
	int x = v.x * (w-1);
	int y = v.y * (h-1);

	float tx = v.x*(w-1)-x;
	float ty = v.y*(h-1)-y;	

	//std::cout << "vectorValue: x: " << x << " y: " << y << " tx: " << tx << " ty: " << ty;

	x = Nimble::Math::Clamp(x, 0, w-1);
	y = Nimble::Math::Clamp(y, 0, h-1);

	Nimble::Vector2 a00(m_vectorFields[0].get(x, y), m_vectorFields[1].get(x, y));
	Nimble::Vector2 a01(m_vectorFields[0].get(x, y+1), m_vectorFields[1].get(x, y+1));
	Nimble::Vector2 a10(m_vectorFields[0].get(x+1, y), m_vectorFields[1].get(x+1, y));
	Nimble::Vector2 a11(m_vectorFields[0].get(x+1, y+1), m_vectorFields[1].get(x+1, y+1));

	Nimble::Vector2 pos = a00*(1-tx)*(1-ty)+a10*tx*(1-ty)+a01*(1-tx)*ty+a11*tx*ty;
	
	//std::cout << "vectorValue pos x: " << pos.x << " pos.y " << pos.y << std::endl;
	return pos;
}

Nimble::Vector2 ReachingWidget::vectorOffset(Nimble::Vector2 pos)
{
	return vectorValue(pos);
}

void ReachingWidget::blur(float ratio)
{
	if (ratio >= 1) return;
	static const int rad = 1;
	static const int count = (rad*2+1)*(rad*2+1);
	for (int y=0; y < h; ++y) {
		for (int x=0; x < w; ++x) {
			for (int k=0; k <= 1; ++k) {
				float sum = 0;
				for (int dy=-rad; dy <= rad; ++dy) {
					for (int dx=-rad; dx <= rad; ++dx) {
						sum += m_vectorFields[k].getSafe(x+dx, y+dy);
					}
				}
				m_vectorFields[k].get(x, y) *= ratio;
				m_vectorFields[k].get(x, y) += (1-ratio)*(sum/count);
			}
		}
	}
}

Nimble::Vector2 ReachingWidget::mapInput(Nimble::Vector2 v)
{
	int x = v.x*w;
	int y = v.y*h;

	float tx = v.x*w-x;
	float ty = v.y*h-y;

	Nimble::Vector2 a00(m_vectorFields[0].get(x, y), m_vectorFields[1].get(x, y));
	Nimble::Vector2 a01(m_vectorFields[0].get(x, y+1), m_vectorFields[1].get(x, y+1));
	Nimble::Vector2 a10(m_vectorFields[0].get(x+1, y), m_vectorFields[1].get(x+1, y));
	Nimble::Vector2 a11(m_vectorFields[0].get(x+1, y+1), m_vectorFields[1].get(x+1, y+1));

	Nimble::Vector2 pos = a00*(1-tx)*(1-ty)+a10*tx*(1-ty)+a01*(1-tx)*ty+a11*tx*ty;
	pos.x *= width();
	pos.y *= height();
	return pos;
}

/// go through all the children, find out if location is inside that widget
MultiWidgets::Widget * ReachingWidget::findChildInside(Luminous::Transformer & tr, Nimble::Vector2f loc, MultiWidgets::Widget * parent)
{
	MultiWidgets::Widget::ChildIterator it = parent->childBegin();
	MultiWidgets::Widget::ChildIterator end = parent->childEnd();

	while (it != end) {
		tr.pushTransformRightMul(it->transform().inverse());
		MultiWidgets::Widget * w = 0;
		if (it->isInside(tr.project(loc))) {
			w = *it;
		} else {
			//w = findChildInside(tr, loc, *it);
		}
		tr.popTransform();
		if (w) return w;
		++it;
	}
	return 0;
}

// is point c left of line which goes through (a,b)
// if determinant of
// [ b.x-a.x c.x-a.x ]
// [ b.y-a.y c.y-a.y ]
// is > 0
bool ReachingWidget::isLeftOfLine(Nimble::Vector2 a, Nimble::Vector2 b, Nimble::Vector2 c)
{
	return (b.x-a.x)*(c.y-a.y) - (c.x-a.x)*(b.y-a.y) > 0;
}
