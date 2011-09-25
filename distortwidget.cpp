#include "distortwidget.hpp"

#include <Luminous/Utils.hpp>
#include <Luminous/FramebufferObject.hpp>
#include <Nimble/Random.hpp>
#include <Radiant/Color.hpp>

#include <Box2D/Box2D.h>

#define INPUT_PULL_NORMAL 0
#define INPUT_PULL_INVERSELY_PROPORTIONAL 1

namespace {
// is point c left of line which goes through (a,b)
// if determinant of
// [ b.x-a.x c.x-a.x ]
// [ b.y-a.y c.y-a.y ]
// is > 0
bool isLeftOfLine(Nimble::Vector2 a, Nimble::Vector2 b, Nimble::Vector2 c)
{
	return (b.x-a.x)*(c.y-a.y) - (c.x-a.x)*(b.y-a.y) > 0;
}
}
struct DistortWidget::GLData : public Luminous::GLResource {
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

DistortWidget::DistortWidget(MultiWidgets::Widget * parent) :
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
	setCSSType("DistortWidget");
	w = h = 0;
	m_decayPerSec = 20.0f;
}


void DistortWidget::deleteChild(Widget *w)
{
	if (m_bodies.count(w)) {
		m_world.DestroyBody(m_bodies[w]);
		m_bodies.erase(m_bodies.find(w));
	}
	MultiWidgets::Widget::deleteChild(w);
}

void DistortWidget::setFeatureFlags(uint32_t flags)
{
	m_featureFlags = flags;
}

void DistortWidget::resize(int width, int height) {
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

void DistortWidget::updateParticles(float dt) {
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

void DistortWidget::decayVectorField(Radiant::MemGrid32f (&field)[2], float dt) {
	float xmove = (m_decayPerSec * dt)/width();
	float ymove = (m_decayPerSec * dt)/height();
	float move = Nimble::Math::Max(xmove, ymove);
	float * tx = field[0].data();
	float * ty = field[1].data();
	for (int y=0; y < h; ++y) {
		for (int x=0; x < w; ++x) {
			/*
			      *tx -= Nimble::Math::Sign(*tx) * std::min(fabsf(*tx), xmove);
			      *ty -= Nimble::Math::Sign(*ty) * std::min(fabsf(*ty), ymove);
			*/
			*tx *= (1 - move);
			*ty *= (1 - move);
			tx++;
			ty++;
		}
	}
}

void DistortWidget::ensureWidgetsHaveBodies() {
	MultiWidgets::Widget::ChildIterator it = childBegin();
	MultiWidgets::Widget::ChildIterator end = childEnd();

	while (it != end) {
		if (m_bodies.count(*it) == 0) {

			std::string type(it->type());
			if(type.compare("clone") == 0) {
				// Do not create shape
				++it;
				continue;
			}

			b2BodyDef bodyDef;
			bodyDef.type = b2_dynamicBody;

			Nimble::Vector2 center = it->mapToParent(0.5f*it->size());
			bodyDef.position.Set(toBox2D(center).x, toBox2D(center).y);

			bodyDef.angle = it->rotation();
			b2Body * body = m_world.CreateBody(&bodyDef);

			Nimble::Vector2 sz = 0.5f * it->size() * it->scale();
			sz.clamp(0.1f, 10000.0f);

			b2FixtureDef fixtureDef;
			b2CircleShape circle;
			//b2PolygonShape box;

			if(type.compare("VacuumWidget") == 0) {
				// Make a bigger shape than the visible widget
				sz *= 1.5f;
				circle.m_radius = toBox2D(sz).x;
				fixtureDef.shape = &circle;
			}
			else {
				//box.SetAsBox(toBox2D(sz).x, toBox2D(sz).y);
				//fixtureDef.shape = &box;
				circle.m_radius = toBox2D(sz).x;
				fixtureDef.shape = &circle;
			}
			fixtureDef.density = Nimble::Math::Max(1.0f/(4*(toBox2D(sz).x*toBox2D(sz).y)), 1e-5f);

			fixtureDef.friction = 0.1f;
			body->CreateFixture(&fixtureDef);
			// Prevent rotation
			body->SetAngularDamping(99.0f);
			m_bodies[*it] = body;
		}
		++it;
	}
}

void DistortWidget::applyForceToBodies(float dt) {
	MultiWidgets::Widget::ChildIterator it = childBegin();
	MultiWidgets::Widget::ChildIterator end = childEnd();

	while (it != end) {
		using Nimble::Vector2;

		static const float timestep = 999.0f; //0.005f;
		static const Vector2 points[] = {
			//Vector2(0.5, 0.5)
			Vector2(0, 0), Vector2(0, 1), Vector2(0.5, 0.5), Vector2(1,0), Vector2(1, 1)
		};
		static const int PointCount = sizeof(points)/sizeof(points[0]);

		Nimble::Vector2 linear;

		if(m_bodies.count(*it) == 0) {
			++it;
			continue;
		}

		b2Body * body = m_bodies[*it];
		body->SetAngularVelocity(0.0f);
		body->SetLinearVelocity(b2Vec2(0,0));

		for (float t = dt; t > 0; t -= timestep) {
			Vector2 vel(0,0);
			for (int i=0; i < PointCount; ++i) {
				Vector2 v = Vector2(points[i].x*it->width(), points[i].y*it->height());
				Vector2 p = it->mapToParent(v);
				p.x /= width();
				p.y /= height();
				p.clampUnit();

				vel = vectorOffset(p);
				vel *= 5;


				if(vel.x > 0.01 || vel.y > 0.01 || vel.x < -0.01 || vel.y < -0.01) {
					(*it)->recursiveSetAlpha(0.5f);
					body->ApplyLinearImpulse(b2Vec2(vel.x, vel.y), body->GetWorldPoint(toBox2D(v)));
				}
				else if(m_vacuumWidgets.size() == 0) {
					// Return to previous location
					Vector2 staticLoc = m_moving_to_static[*it]->location();

					(*it)->recursiveSetAlpha(0.0f);
					if(m_bodies.count(*it) > 0 && ((*it)->location() - staticLoc).length() > 0.1){
						m_world.DestroyBody(body);
						m_bodies.erase(m_bodies.find(*it));
						(*it)->setRotation(m_moving_to_static[*it]->rotation());
						(*it)->setLocation(m_moving_to_static[*it]->location());
						
					}
				}
				//body->ApplyLinearImpulse(b2Vec2(vel.x, vel.y), body->GetWorldPoint(toBox2D(v)));
			}
		}
		++it;
	}
}

void DistortWidget::ensureGroundInitialized() {
	static bool groundInit = false;
	const float border = 1.0f;
	if (!groundInit) {
		groundInit = true;
		b2BodyDef groundDef;
		b2Vec2 c = toBox2D(size() * 0.5f);
		groundDef.position.Set(c.x, c.y);
		groundBody = m_world.CreateBody(&groundDef);
		b2PolygonShape groundBox;
		//groundBox.SetAsEdge(b2Vec2(-c.x, -c.y), b2Vec2(-c.x, c.y));
		groundBox.SetAsBox(border, c.y + border, b2Vec2(-c.x - border, 0.0f), 0.0f);
		groundBody->CreateFixture(&groundBox, 0.0f);
		//groundBox.SetAsEdge(b2Vec2(-c.x, c.y), b2Vec2(c.x, c.y));
		groundBox.SetAsBox(c.x, border, b2Vec2(0.0f, c.y + border), 0.0f);
		groundBody->CreateFixture(&groundBox, 0.0f);
		//groundBox.SetAsEdge(b2Vec2(c.x, c.y), b2Vec2(c.x, -c.y));
		groundBox.SetAsBox(border, c.y + border, b2Vec2(c.x + border, 0.0f), 0.0f);
		groundBody->CreateFixture(&groundBox, 0.0f);
		//groundBox.SetAsEdge(b2Vec2(c.x, -c.y), b2Vec2(-c.x, -c.y));
		groundBox.SetAsBox(c.x, border, b2Vec2(0.0f, -c.y - border), 0.0f);
		groundBody->CreateFixture(&groundBox, 0.0f);
	}
}
void DistortWidget::addStaticDisk(void* id, Nimble::Vector2 center, float rad)
{
	removeStaticObject(id);

	b2BodyDef def;
	def.type = b2_staticBody;
	def.position.Set(toBox2D(center).x, toBox2D(center).y);

	b2Body * body = m_world.CreateBody(&def);

	b2CircleShape circle;
	circle.m_p = b2Vec2(0,0); //toBox2D(center);
	circle.m_radius = toBox2D(rad);
	b2FixtureDef fixtureDef;

	body->CreateFixture(&circle, 0.0f);
	m_statics[id] = body;
}

void DistortWidget::removeStaticObject(void *id)
{
	std::map<void*, b2Body*>::iterator it = m_statics.find(id);
	if (it == m_statics.end())
		return;


	m_world.DestroyBody(it->second);
	m_statics.erase(it);
}

void DistortWidget::addStaticBox(const Nimble::Rectangle& rect)
{
	b2BodyDef groundDef;
	b2Vec2 c = toBox2D(rect.center());
	groundDef.position.Set(c.x, c.y);
	b2Body * groundBody = m_world.CreateBody(&groundDef);
	b2PolygonShape groundBox;
	std::vector<Nimble::Vector2> corners;
	rect.computeCorners(corners);
	for (unsigned int i=0; i < corners.size(); ++i)
		corners[i] = rect.center() - corners[i];

	int n = corners.size();
	for (unsigned int i=0; i < corners.size(); ++i) {
		groundBox.SetAsEdge(toBox2D(corners[i]), toBox2D(corners[(i+1)%n]));
		/*
				b2Vec2(corners[i].x, corners[i].y),
			b2Vec2(corners[(i+1)%n].x, corners[(i+1)%n].y));
			*/
		groundBody->CreateFixture(&groundBox, 0.0f);
	}
}

void DistortWidget::addOutsideWidget(MultiWidgets::Widget * w, float density, float friction) {
	if (m_bodies.count(w) > 0)
		return;

	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;

	Nimble::Vector2 center = w->mapToParent(0.5f*w->size());
	bodyDef.position.Set(toBox2D(center).x, toBox2D(center).y);

	bodyDef.angle = w->rotation();
	b2Body * body = m_world.CreateBody(&bodyDef);

	Nimble::Vector2 sz = 0.5f * w->size() * w->scale();
	sz.clamp(0.1f, 10000.0f);
	b2PolygonShape box;
	box.SetAsBox(toBox2D(sz).x, toBox2D(sz).y);

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &box;
	fixtureDef.density = Nimble::Math::Max(1.0f/(4*(toBox2D(sz).x*toBox2D(sz).y)), 1e-5f);
	fixtureDef.density = density;

	fixtureDef.friction = friction;
	body->CreateFixture(&fixtureDef);
	m_bodies[w] = body;
}

void DistortWidget::updateBodiesToWidgets() {
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
		//std::cout << it->second->GetMa() << std::endl;
	}
}

void DistortWidget::processGhosts()
{
	
}

void DistortWidget::update(float dt)
{
	MultiWidgets::Widget::update(dt);
	/*
	float coeff = 1.0f;
	if (lastInteraction() != 0) {
	  coeff = Nimble::Math::Min(lastInteraction().sinceSecondsD(), 100.0);
	}
	*/
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

	processGhosts();

	if (m_featureFlags & FEATURE_GRAVITY)
		m_world.SetGravity(b2Vec2(m_gravity.x(), m_gravity.y()));

	for (int i=0; i < 1; ++i) {
		m_world.Step(dt/1, 10, 10);
	}
	m_world.ClearForces();

	updateBodiesToWidgets();
}

Nimble::Vector2 DistortWidget::vectorValue(Nimble::Vector2 v)
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
	
	//if(pos.x < 0.00001) pos.x = 0;
	//if(pos.y < 0.00001) pos.y = 0;
	//std::cout << "vectorValue pos x: " << pos.x << " pos.y " << pos.y << std::endl;
	return pos;
}

Nimble::Vector2 DistortWidget::vectorOffset(Nimble::Vector2 pos)
{
	return vectorValue(pos);
}

void DistortWidget::blur(float ratio)
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

Nimble::Vector2 DistortWidget::mapInput(Nimble::Vector2 v)
{
	int x = v.x*w;
	int y = v.y*h;

	float tx = v.x*w-x;
	float ty = v.y*h-y;

	Nimble::Vector2 a00(m_vectorFields[0].get(x, y), m_vectorFields[1].get(x, y));
	Nimble::Vector2 a01(m_vectorFields[0].get(x, y+1), m_vectorFields[1].get(x, y+1));
	Nimble::Vector2 a10(m_vectorFields[0].get(x+1, y), m_vectorFields[1].get(x+1, y));
	Nimble::Vector2 a11(m_vectorFields[0].get(x+1, y+1), m_vectorFields[1].get(x+1, y+1));

	//    return a00;
	Nimble::Vector2 pos = a00*(1-tx)*(1-ty)+a10*tx*(1-ty)+a01*(1-tx)*ty+a11*tx*ty;
	pos.x *= width();
	pos.y *= height();
	//    pos.y = height() - height() * pos.y;
	return pos;
}

/// go through all the children, find out if location is inside that widget
MultiWidgets::Widget * DistortWidget::findChildInside(Luminous::Transformer & tr, Nimble::Vector2f loc, MultiWidgets::Widget * parent)
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

void DistortWidget::addVacuumWidget(long fingerId, Nimble::Vector2 center)
{
	//std::cout << "Adding vacuum widget" << std::endl;
	VacuumWidget * v = new VacuumWidget(this);
	//v->setStyle(style());
	m_vacuumWidgets[fingerId] = v;

	v->setThickness(10);
	v->setSize(Nimble::Vector2(100, 100));
	v->setColor(Radiant::Color("#69e8ffdd"));
	v->setDepth(-1);
	v->raiseFlag(VacuumWidget::LOCK_DEPTH);
	v->setCenterLocation(center);
	v->setInputTransparent(true);
}

void DistortWidget::deleteVacuumWidget(long fingerId)
{
	if(m_vacuumWidgets.count(fingerId) == 0) return;
	VacuumWidget * v = m_vacuumWidgets[fingerId];
	m_vacuumWidgets.erase(fingerId);
	if(m_bodies.count(v) > 0) {
		m_world.DestroyBody(m_bodies[v]);
		m_bodies.erase(m_bodies.find(v));
	}
	deleteChild(v);
}

void DistortWidget::modifyVacuumWidget(long fingerId, double rotation, float intensity) {
	if(m_vacuumWidgets.count(fingerId) == 0) return;
	VacuumWidget * v = m_vacuumWidgets[fingerId];
	v->setRotationAboutCenter(rotation);
	//std::cout << "intensity: " << intensity << std::endl;
	intensity = Nimble::Math::Clamp(intensity, 0.0f, 0.4f);
	if(intensity < 0.028) {
		intensity = 0;
	}
	//std::cout << "m_arc: " << intensity << std::endl;
	v->setArc(intensity);
}

void DistortWidget::input(MultiWidgets::GrabManager & gm, float /*dt*/)
{
	gm.pushTransformRightMul(transform());
	Nimble::Matrix3 m = gm.transform();

	MultiTouch::Sample s1 = gm.prevSample();
	MultiTouch::Sample s2 = gm.sample();
	std::vector<long> lost;

	for (std::set<long>::iterator it = m_currentFingerIds.begin(); it != m_currentFingerIds.end(); ) {
		if (s2.findFinger(*it).isNull()) {
			lost.push_back(*it);
			// Delete vacuum widget possibly activated by the lost finger
			deleteVacuumWidget(*it);
			m_currentFingerIds.erase(it++);
		} else {
			it++;
		}
	}

	for (unsigned int i=0; i < lost.size(); ++i) {
		MultiTouch::Finger f = s1.findFinger(lost[i]);
		if (f.isNull()) continue;
		Luminous::Transformer tr;
		tr.pushTransform(Nimble::Matrix3::IDENTITY);

		MultiWidgets::Widget * hit = findChildInside(tr, gm.project(f.initTipLocation()), this);
		MultiWidgets::Widget * hit2 = findChildInside(tr, gm.project(f.tipLocation()), this);
		const float moveThreshold = 30*30;
		if (hit && strcmp(hit->type(), "VacuumWidget") != 0 && (hit == hit2 || (f.tipLocation() - f.initTipLocation()).lengthSqr() < moveThreshold)) {
			m_world.DestroyBody(m_bodies[hit] );
			m_bodies.erase(m_bodies.find(hit));
			hit->touch();
			hit->setDepth(-20);

			/*			
			MultiWidgets::Widget * staticList = m_moving_to_static[hit];
			m_static_to_moving.erase(staticList);
			m_moving_to_static.erase(hit);
			deleteChild(staticList);
			*/

			hit->recursiveSetAlpha(1.0f);
			parent()->addChild(hit);
		}
	}

	int n = gm.fingerCount();
	for(int i = 0; i < n; i++) {
		MultiTouch::Finger f = gm.getFinger(i);
		// don't process grabbed fingers
		if (gm.fingerGrabber(f.id()))
			continue;

		m_currentFingerIds.insert(f.id());

		Nimble::Vector2 loc = (m * f.tipLocation()).vector2();
		Nimble::Vector2 nLoc(loc.x/width(), loc.y/height());
		Nimble::Vector2 nHandLoc = loc; //gm.findHand(f.handId()).palmCenter();

		nHandLoc.x /= width();
		nHandLoc.y /= height();

		// Use initial for vacuum instead of prev

		Nimble::Vector2 locInit = (m * f.initTipLocation()).vector2();
		Nimble::Vector2 locPrev = (m * f.prevTipLocation()).vector2();

		//Nimble::Vector2 diff = loc - locPrev;
		// Changed to locInit - loc for vacuum
		//Nimble::Vector2 diff = locInit - loc;
		Nimble::Vector2 diff = loc - locInit;

		//if (f.age()/gm.touchScreen().framesPerSecond() > 0.3f) {
		Luminous::Transformer tr;
		tr.pushTransform(Nimble::Matrix3::IDENTITY);
		MultiWidgets::Widget * hit = findChildInside(tr, gm.project(f.initTipLocation()), this);
		MultiWidgets::Widget * hit2 = findChildInside(tr, gm.project(f.tipLocation()), this);
		if (hit && hit == hit2 && strcmp(hit2->type(), "VacuumWidget") != 0 && strcmp(hit2->type(), "clone") != 0) {
			if(m_bodies.count(hit) > 0)
			{
				m_world.DestroyBody(m_bodies[hit] );
				m_bodies.erase(m_bodies.find(hit));
			}
			hit->touch();
			
			if(m_moving_to_static.count(hit) > 0)
			{
				m_moving_to_static[hit]->hide();
				MultiWidgets::Widget * staticList = m_moving_to_static[hit];
				staticList->hide();
				m_static_to_moving.erase(staticList);
				m_moving_to_static.erase(hit);
				//std::cout << "deleteChild" << std::endl;
				deleteChild(staticList);
				hit->recursiveSetAlpha(1.0f);
			}
			
			parent()->addChild(hit);
			continue;
		}
		//}

		touch();

		//std::cout << "diff.length: " << diff.length();

		// Do not activate unless threshold is enough

		if (diff.length() < 20 || f.age()/gm.touchScreen().framesPerSecond() < 0.3f) {
			continue;
		}

		if (!(m_featureFlags & FEATURE_VELOCITY_FIELD))
			return;

		//std::cout << "Finger id: " << f.id() << std::endl;
		//std::cout << "f.age(): " << f.age() << std::endl;

		using namespace Nimble;

		Nimble::Vector2 coord;
		Nimble::Vector2 nDiff = diff * -1;
		nDiff.normalize();

		//float lengthMultiplier = 1.0;//m_input_pull_type == INPUT_PULL_INVERSELY_PROPORTIONAL ? m_input_pull_intensity : 1.0f/m_input_pull_intensity;

		Nimble::Vector2 l1[2];
		Nimble::Vector2 l2[2];
		Nimble::Vector2 perp = nDiff.perpendicular();

		// In vacuum diff must influence the angle
		perp.normalize(m_tubeWidth * (diff.length()/50));

		Nimble::Vector2 nLocInit(locInit.x/width(), locInit.y/height());
		/*
		    l1[0] = nHandLoc + perp;
		    l1[1] = nHandLoc - perp;
		    l2[0] = nHandLoc + nDiff + perp;
		    l2[1] = nHandLoc + nDiff - perp;
		*/

		l1[0] = nLocInit + perp;
		l1[1] = nLocInit - perp;
		l2[0] = nLocInit + nDiff + perp;
		l2[1] = nLocInit + nDiff - perp;

		for (int y=0; y < h; ++y) {
			float * p1 = m_vectorFields[0].line(y);
			float * p2 = m_vectorFields[1].line(y);
			coord.y = y/float(h-1);

			for (int x=0; x < w; ++x) {
				coord.x = x/float(w-1);
				// User nLocInit instead of nHandLoc
				Nimble::Vector2 dir = nLocInit - coord;
				Nimble::Vector2 nDir = dir;
				nDir.normalize();

				float angle = Math::ACos(Nimble::dot(nDiff, dir/dir.length()));
				float nn = 1.0 - angle/(m_input_pull_angle);

				if (nn < 0 || isLeftOfLine(l1[0], l2[0], coord) || !isLeftOfLine(l1[1], l2[1], coord)) {
					//point is not within area of influence
					p1++;
					p2++;
					continue;
				}

				//float len = dir.length()*lengthMultiplier;

				//nn = Math::Min(nn, 1.0f);
				//float magnitude = nn*(m_input_pull_type == INPUT_PULL_INVERSELY_PROPORTIONAL ? len : 1.0f) * m_input_pull_intensity;
				//float moveX = (diff.x*magnitude)/width();
				//float moveY = (diff.y*magnitude)/height();

				// Vacuum uses constant velocity (normalized)

				float moveX = (nDir.x * 100)/width() * m_input_pull_intensity;
				float moveY = (nDir.y * 100)/height() * m_input_pull_intensity;

				//*p1 = Math::Clamp(*p1 + Math::Min(moveX, 0.2f), -1.0f, 1.0f);
				//*p2 = Math::Clamp(*p2 + Math::Min(moveY, 0.2f), -1.0f, 1.0f);
				*p1 += moveX;
				*p2 += moveY;

				p1++;
				p2++;
			}
		}

		// Draw vacuum widget if not yet drawn
		if(m_vacuumWidgets.count(f.id()) == 0) {
			addVacuumWidget(f.id(), f.initTipLocation());
		}
		else
		{
			//std::cout << "diff.length() " << Nimble::Math::Sqrt(diff.length()-20) << std::endl;
			modifyVacuumWidget(f.id(), diff.angle(), Nimble::Math::Sqrt(diff.length()-20)/50);
		}
	}
	gm.popTransform();
}

void DistortWidget::render(Luminous::RenderContext & r)
{
	const Luminous::MultiHead::Area * area;
	Luminous::GLResources::getThreadMultiHead(0, &area);
	const Nimble::Vector2f gfxLoc = area->graphicsLocation();

	GLRESOURCE_ENSURE(GLData, gld, this, r.resources());

	Luminous::Texture2D* textures[2] = {&gld->m_tex[0].ref(), &gld->m_tex[1].ref()};
	Luminous::Texture2D & textureRead = gld->m_tex[2].ref();
	Luminous::Texture2D & particleTex = gld->m_particleTexture.ref();

	if (particleTex.generation() < 0) {
		particleTex.loadImage("particle.png");
		particleTex.setGeneration(1);
	}

	r.pushTransformRightMul(transform());
	glDisable(GL_TEXTURE_2D);

	//renderChildren(r);

	Nimble::Vector2i texSize(w, h);
	for (int i=0; i < 2; ++i) {
		textures[i]->bind(GL_TEXTURE1 + i);
		textures[i]->loadBytes(GL_LUMINANCE32F_ARB,
		                       w, h, m_vectorFields[i].data(),
		                       Luminous::PixelFormat::luminanceFloat(),
		                       false);

	}

	glEnable(GL_TEXTURE_2D);
	textureRead.bind(GL_TEXTURE0);
	if (textureRead.size() != area->size()) {
		textureRead.loadBytes(GL_RGBA, area->width(), area->height(), 0,
		                      Luminous::PixelFormat::rgbaUByte(), false);
	}

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gld->m_fbo->bind();
	gld->m_fbo->attachTexture2D(&textureRead, Luminous::COLOR0);

	gld->m_fbo->check();
	glDrawBuffer(Luminous::COLOR0);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, area->width(), area->height());


	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	Luminous::Utils::glUsualBlend();

	Nimble::Matrix3 m = Nimble::Matrix3::scale2D(1, -1) * Nimble::Matrix3::translate2D(0, -area->graphicsSize().y);
	// render
	if (m_featureFlags & FEATURE_PARTICLES && !m_particles.empty()) {
		r.pushTransformRightMul(m * Nimble::Matrix3::scale2D(width(), height()));
		gld->m_vbo->allocate(m_particles.size()*2*sizeof(Particle), Luminous::VertexBuffer::STREAM_DRAW);
		Particle * v = (Particle*)gld->m_vbo->map(Luminous::VertexBuffer::WRITE_ONLY);
		memcpy(v, &m_particles[0], sizeof(Particle)*m_particles.size());
		// draw particles
		//        glDisable(GL_TEXTURE_2D);
//    glEnable(GL_POINT_SMOOTH);
		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

		glEnable(GL_POINT_SPRITE);
		glPointSize(32);
		glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

		particleTex.bind(GL_TEXTURE0);

		gld->m_vbo->unmap();
		glColor4f(0.255, 0.4, 0.96, 0.5);

		gld->m_vbo->bind();

		GLuint ageLoc = gld->m_particleShader->getAttribLoc("age");
		glEnableVertexAttribArray(ageLoc);
		GLuint maxAgeLoc = gld->m_particleShader->getAttribLoc("max_age");
		glEnableVertexAttribArray(maxAgeLoc);
		GLuint brightnessLoc = gld->m_particleShader->getAttribLoc("brightness");
		glEnableVertexAttribArray(brightnessLoc);

		glEnableClientState(GL_VERTEX_ARRAY);
		gld->m_particleShader->bind();
		gld->m_particleShader->setUniformInt("tex", 0);
		gld->m_particleShader->setUniformInt("fieldX", 1);
		gld->m_particleShader->setUniformInt("fieldY", 2);
		Nimble::Matrix3 tmp = r.transform();
		gld->m_particleShader->setUniformMatrix3("transformation", tmp);
		glVertexPointer(2, GL_FLOAT, sizeof(Particle), BUFFER_OFFSET(sizeof(float)*3));
		glVertexAttribPointer(ageLoc, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), BUFFER_OFFSET(sizeof(float)*0));
		glVertexAttribPointer(maxAgeLoc, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), BUFFER_OFFSET(sizeof(float)*1));
		glVertexAttribPointer(brightnessLoc, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), BUFFER_OFFSET(sizeof(float)*2));

		glDrawArrays(GL_POINTS, 0, m_particles.size());
		gld->m_particleShader->unbind();

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableVertexAttribArray(ageLoc);
		glDisableVertexAttribArray(maxAgeLoc);
		glDisableVertexAttribArray(brightnessLoc);

		gld->m_vbo->unbind();

		r.popTransform();
	}

	glPopAttrib();
	gld->m_fbo->unbind();
	glDrawBuffer(GL_BACK);

	glEnable(GL_TEXTURE_2D);

	textureRead.bind(GL_TEXTURE0);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	gld->m_shader->bind();
	gld->m_shader->setUniformInt("background", 0);
	gld->m_shader->setUniformInt("fieldX", 1);
	gld->m_shader->setUniformInt("fieldY", 2);
	gld->m_shader->setUniformVector2("size", area->size());
	//gld->m_shader->setUniformVector2("fieldSize", Nimble::Vector2f(w, h));

	Luminous::Utils::glUsualBlend();
	//r.drawTexRect(area->graphicsBounds(), Nimble::Vector4(1, 1, 1, 1).data());


	Luminous::Utils::glTexRect(area->graphicsSize(), r.transform() * Nimble::Matrix3::translate2D(gfxLoc.x, gfxLoc.y));

	gld->m_shader->unbind();

	// hack around macintosh opengl strangeness
	textures[0]->bind(GL_TEXTURE2);
	//    glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);
	textures[0]->bind(GL_TEXTURE1);
	//    glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	textures[0]->bind(GL_TEXTURE0);
	//    glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);


	if (m_featureFlags & FEATURE_ARROWS) {
		glBegin(GL_LINES);
		// draw arrows for vector field
		for (int y=0; y < h; ++y) {
			for (int x=0; x < w; ++x) {
				Nimble::Vector2 loc(m_vectorFields[0].get(x,y)*width(), m_vectorFields[1].get(x,y)*height());
				Nimble::Vector2 id(float(x)/(w-1) * width(), float(y)/(h-1) * height());
				loc = r.project(loc);
				id = r.project(id);
				Nimble::Vector2 off = loc;
				float lSq = off.lengthSqr();
				if (lSq < 0.5f) continue;
				off.normalize(2.0f*Nimble::Math::Log2(lSq));

				glColor4f(0.0, 0.0, 1.0, 0.3);
				glVertex2fv(id.data());
				glColor4f(0.0, 0.0, 1.0, 0.9);
				Nimble::Vector2 end = id+off;
				glVertex2fv(end.data());
				// arrow head
				Nimble::Vector2 right = off;
				right.rotate(3*Nimble::Math::PI/4);
				right.normalize(Nimble::Math::Clamp(off.length(), 1.0f, 5.0f));
				glVertex2fv(end.data());
				glColor4f(0.0, 0.0, 1.0, 0.3);
				glVertex2fv((end+right).data());
				glColor4f(0.0, 0.0, 1.0, 0.9);
				glVertex2fv(end.data());
				glColor4f(0.0, 0.0, 1.0, 0.3);
				glVertex2fv((end+right.perpendicular()).data());
			}
		}
		glEnd();
	}

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	gld->m_fbo->bind();
	gld->m_fbo->attachTexture2D(&textureRead, Luminous::COLOR0);
	gld->m_fbo->check();
	glDrawBuffer(Luminous::COLOR0);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, area->width(), area->height());
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	r.pushTransformRightMul(m);
	// Commented out in vacuum
	//renderChildren(r);
	r.popTransform();

	glPopAttrib();
	gld->m_fbo->unbind();
	textureRead.bind(GL_TEXTURE0);

	Luminous::Utils::glUsualBlend();
	//glColor4f(0, 0, 0, 0.9);

	//r.drawTexRect(area->graphicsBounds(), Nimble::Vector4(1, 1, 1, 1).data());
	//Luminous::Utils::glTexRect(area->graphicsSize(), r.transform() * Nimble::Matrix3::translate2D(gfxLoc.x, gfxLoc.y));

	// Changed to here in vacuum
	renderChildren(r);

	// hack around macintosh opengl strangeness
	textures[0]->bind(GL_TEXTURE2);
	//    glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);
	textures[0]->bind(GL_TEXTURE1);
	//    glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	textures[0]->bind(GL_TEXTURE0);
	//    glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
	r.popTransform();
}



