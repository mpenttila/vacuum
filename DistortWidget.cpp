#include "DistortWidget.hpp"

#include <Luminous/Utils.hpp>
#include <Luminous/FramebufferObject.hpp>
#include <Nimble/Random.hpp>

#include <Box2D/Box2D.h>

#define INPUT_PULL_NORMAL 0
#define INPUT_PULL_INVERSELY_PROPORTIONAL 1


DistortWidget::DistortWidget(MultiWidgets::Widget * parent) :
		ReachingWidget(parent)
{
	setCSSType("DistortWidget");
}

void DistortWidget::ensureWidgetsHaveBodies() {
	MultiWidgets::Widget::ChildIterator it = childBegin();
	MultiWidgets::Widget::ChildIterator end = childEnd();

	while (it != end) {
		if (m_bodies.count(*it) == 0) {
			b2BodyDef bodyDef;
			bodyDef.type = b2_dynamicBody;

			Nimble::Vector2 center = it->mapToParent(0.5f*it->size());
			bodyDef.position.Set(toBox2D(center).x, toBox2D(center).y);

			bodyDef.angle = it->rotation();
			b2Body * body = m_world.CreateBody(&bodyDef);

			Nimble::Vector2 sz = 0.5f * it->size() * it->scale();
			sz.clamp(0.1f, 10000.0f);
			b2PolygonShape box;
			box.SetAsBox(toBox2D(sz).x, toBox2D(sz).y);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &box;
			fixtureDef.density = Nimble::Math::Max(1.0f/(4*(toBox2D(sz).x*toBox2D(sz).y)), 1e-5f);

			fixtureDef.friction = 0.1f;
			body->CreateFixture(&fixtureDef);
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

				body->ApplyLinearImpulse(b2Vec2(vel.x, vel.y), body->GetWorldPoint(toBox2D(v)));
			}
		}
		++it;
	}
}

void DistortWidget::input(MultiWidgets::GrabManager & gm, float /* dt */)
{
	gm.pushTransformRightMul(transform());
	Nimble::Matrix3 m = gm.transform();

	MultiTouch::Sample s1 = gm.prevSample();
	MultiTouch::Sample s2 = gm.sample();
	std::vector<long> lost;

	for (std::set<long>::iterator it = m_currentFingerIds.begin(); it != m_currentFingerIds.end(); ) {
		if (s2.findFinger(*it).isNull()) {
			lost.push_back(*it);
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
		if (hit && (hit == hit2 || (f.tipLocation() - f.initTipLocation()).lengthSqr() < moveThreshold)) {
			m_world.DestroyBody(m_bodies[hit] );
			m_bodies.erase(m_bodies.find(hit));
			hit->touch();
			hit->setDepth(-20);
			parent()->addChild(hit);
		}
	}

	int n = gm.fingerCount();
	for (int i = 0; i < n; i++) {
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

		Nimble::Vector2 locPrev = (m * f.prevTipLocation()).vector2();
		Nimble::Vector2 diff = loc - locPrev;

		//if (f.age()/gm.touchScreen().framesPerSecond() > 0.3f) {
		Luminous::Transformer tr;
		tr.pushTransform(Nimble::Matrix3::IDENTITY);
		MultiWidgets::Widget * hit = findChildInside(tr, gm.project(f.initTipLocation()), this);
		MultiWidgets::Widget * hit2 = findChildInside(tr, gm.project(f.tipLocation()), this);
		if (hit && hit == hit2) {
			m_world.DestroyBody(m_bodies[hit] );
			m_bodies.erase(m_bodies.find(hit));
			hit->touch();
			parent()->addChild(hit);
			continue;
		}
		//}

		touch();

		if (diff.length() < 1e-7) {
			continue;
		}

		if (!(m_featureFlags & FEATURE_VELOCITY_FIELD))
			return;

		using namespace Nimble;

		Nimble::Vector2 coord;
		Nimble::Vector2 nDiff = diff;
		nDiff.normalize();

		float lengthMultiplier = 1.0;//m_input_pull_type == INPUT_PULL_INVERSELY_PROPORTIONAL ? m_input_pull_intensity : 1.0f/m_input_pull_intensity;

		Nimble::Vector2 l1[2];
		Nimble::Vector2 l2[2];
		Nimble::Vector2 perp = nDiff.perpendicular();
		perp.normalize(m_tubeWidth);
		l1[0] = nHandLoc + perp;
		l1[1] = nHandLoc - perp;
		l2[0] = nHandLoc + nDiff + perp;
		l2[1] = nHandLoc + nDiff - perp;

		for (int y=0; y < h; ++y) {
			float * p1 = m_vectorFields[0].line(y);
			float * p2 = m_vectorFields[1].line(y);
			coord.y = y/float(h-1);

			for (int x=0; x < w; ++x) {
				coord.x = x/float(w-1);
				Nimble::Vector2 dir = nHandLoc - coord;

				float angle = Math::ACos(Nimble::dot(nDiff, dir/dir.length()));
				float nn = 1.0 - angle/(m_input_pull_angle);
//         if (nn < 0 || isLeftOfLine(l1[0], l2[0], coord)) {
				if (nn < 0 || isLeftOfLine(l1[0], l2[0], coord) || !isLeftOfLine(l1[1], l2[1], coord)) {
					p1++;
					p2++;
					continue;
				}

				float len = dir.length()*lengthMultiplier;

				nn = Math::Min(nn, 1.0f);
				float magnitude = nn*(m_input_pull_type == INPUT_PULL_INVERSELY_PROPORTIONAL ? len : 1.0f) * m_input_pull_intensity;
				float moveX = (diff.x*magnitude)/width();
				float moveY = (diff.y*magnitude)/height();
				//*p1 = Math::Clamp(*p1 + Math::Min(moveX, 0.2f), -1.0f, 1.0f);
				//*p2 = Math::Clamp(*p2 + Math::Min(moveY, 0.2f), -1.0f, 1.0f);
				*p1 += moveX;
				*p2 += moveY;

				p1++;
				p2++;
			}
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
/*	if (m_featureFlags & FEATURE_PARTICLES && !m_particles.empty()) {
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
*/
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

	//renderChildren(r);
	r.popTransform();

	glPopAttrib();
	gld->m_fbo->unbind();
	textureRead.bind(GL_TEXTURE0);

	Luminous::Utils::glUsualBlend();
	//glColor4f(1, 1, 1, m_bg_alpha);

	//r.drawTexRect(area->graphicsBounds(), Nimble::Vector4(1, 1, 1, 1).data());
	//Luminous::Utils::glTexRect(area->graphicsSize(), r.transform() * Nimble::Matrix3::translate2D(gfxLoc.x, gfxLoc.y));

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

void DistortWidget::addMovingAndStaticWidgetPair(MultiWidgets::Widget*, MultiWidgets::Widget*)
{
	// Do nothing
}

