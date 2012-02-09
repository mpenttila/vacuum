#include "VacuumReachingWidget.hpp"
#include "RoundTextBox.hpp"


#include <Luminous/Utils.hpp>
#include <Luminous/FramebufferObject.hpp>
#include <Nimble/Random.hpp>
#include <Radiant/Color.hpp>
#include <MultiWidgets/MessageSendOperator.hpp>
#include <Radiant/BinaryData.hpp>
#include <Nimble/Math.hpp>

#include <Box2D/Box2D.h>


VacuumReachingWidget::VacuumReachingWidget(MultiWidgets::Widget * parent) :
	ReachingWidget(parent)
{
	setCSSType("VacuumReachingWidget");
}

void VacuumReachingWidget::ensureWidgetsHaveBodies() {
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
			fixtureDef.density = Nimble::Math::Max(1.0f/((toBox2D(sz).x*toBox2D(sz).y)), 0.1f);
			//std::cout << "density: " << fixtureDef.density << std::endl;

			fixtureDef.friction = 0.1f;
			body->CreateFixture(&fixtureDef);
			// Prevent rotation
			body->SetAngularDamping(99.0f);
			m_bodies[*it] = body;
		}
		++it;
	}
}

void VacuumReachingWidget::applyForceToBodies(float dt) {
	MultiWidgets::Widget::ChildIterator it = childBegin();
	MultiWidgets::Widget::ChildIterator end = childEnd();

	while (it != end) {
		using Nimble::Vector2;

		static const float timestep = 999.0f; //0.005f;
		static const Vector2 points[] = {
                        Vector2(0.5, 0.5)
                        //Vector2(0, 0), Vector2(0, 1), Vector2(0.5, 0.5), Vector2(1,0), Vector2(1, 1)
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

void VacuumReachingWidget::applyVacuum(){

    using namespace Nimble::Math;

    MultiWidgets::Widget::ChildIterator it = childBegin();
    MultiWidgets::Widget::ChildIterator end = childEnd();

    while (it != end) {

        std::string type(it->type());
        if(type.compare("RoundTextBox") != 0) {
            // Do not create shape
            ++it;
            continue;
        }

        RoundTextBox * tb = dynamic_cast<RoundTextBox*>((*it));

        using Nimble::Vector2;

        static const Vector2 points[] = {
            Vector2(0, 0.5), Vector2(0.5, 0), Vector2(0.5, 1), Vector2(1, 0.5)
        };
        static const int PointCount = sizeof(points)/sizeof(points[0]);


//        if(m_bodies.count(*it) == 0) {
//            ++it;
//            continue;
//        }

//        b2Body * body = m_bodies[*it];
//        body->SetAngularVelocity(0.0f);
//        body->SetLinearVelocity(b2Vec2(0,0));

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
                std::map<long, VacuumWidget *>::const_iterator vaciter;
                for(vaciter = m_vacuumWidgets.begin(); vaciter != m_vacuumWidgets.end(); ++vaciter  ){
                    VacuumWidget * vw = vaciter->second;
                    if(m_vacuumClones.count(*it) == 0){
                        RoundTextBox * clone = tb->clone();
                        float scalingFactor = 0.4;
                        float cloneWidth = scalingFactor * tb->width();
                        clone->setScale(scalingFactor);
                        int vwx = vw->sceneGeometry().center().x;
                        int vwy = vw->sceneGeometry().center().y;
                        int tbx = tb->location().x + tb->width()/2;
                        int tby = tb->location().y + tb->width()/2;
                        int x = floor(Cos(vw->rotation() ) * (vw->width()/2 + Abs((vwx - tbx)*0.3)) +
                                      vwx);
                        int y = floor(Sin(vw->rotation() ) * (vw->width()/2 + 100 + Abs((vwy - tby)*0.3)) +
                                      vwy);
                        //std::cout << "rotation: " << vw->rotation() << " vwx: " << vwx << " tbx: " << tbx << " x: " << x << std::endl;
                        //std::cout << " vwy: " << vwy << " tby: " << tby << " y: " << y << std::endl;
                        clone->setLocation(x - cloneWidth/2, y - cloneWidth/2);
                        //std::cout << "clone location: x: " << clone->location().x << " y: " << clone->location().y << std::endl;
                        tb->recursiveSetAlpha(0.4f);
                        clone->setType("clone");
                        std::string eventname = std::string("word-acquired-") + Radiant::StringUtils::stringify(tb->player());
                        clone->eventAddListener("interactionbegin", eventname.c_str(), wordGameWidget);
                        m_vacuumClones[*it] = clone;
                        tb->setDepth(-11);
                        clone->setDepth(-10);
                        std::cout << tb->depth() << " " << clone->depth() << " vacuum: " << vw->depth() << std::endl;
                    }
                }
            }
            else {
                if(m_vacuumClones.count(*it) > 0){
                    deleteChild(m_vacuumClones[*it]);
                    m_vacuumClones.erase(*it);
                    (*it)->recursiveSetAlpha(1.0f);
                }
            }
            //body->ApplyLinearImpulse(b2Vec2(vel.x, vel.y), body->GetWorldPoint(toBox2D(v)));
        }
        ++it;
    }
}

void VacuumReachingWidget::addVacuumWidget(long fingerId, Nimble::Vector2 center, double rotation)
{
	//std::cout << "Adding vacuum widget" << std::endl;
	VacuumWidget * v = new VacuumWidget(this);
	//v->setStyle(style());
	m_vacuumWidgets[fingerId] = v;

    v->setThickness(5);
	v->setSize(Nimble::Vector2(100, 100));
	v->setColor(Radiant::Color("#69e8ffdd"));
    v->setDepth(-1);
    v->raiseFlag(VacuumWidget::LOCK_DEPTH);
	v->setCenterLocation(center);
	v->setInputTransparent(true);
    v->setRotationAboutCenter(rotation);
}

void VacuumReachingWidget::deleteVacuumWidget(long fingerId)
{
	if(m_vacuumWidgets.count(fingerId) == 0) return;
	VacuumWidget * v = m_vacuumWidgets[fingerId];
	m_vacuumWidgets.erase(fingerId);
	if(m_bodies.count(v) > 0) {
        m_world.DestroyBody(m_bodies[v]);
		m_bodies.erase(m_bodies.find(v));
	}
	deleteChild(v);
    if(m_vacuumWidgets.size() == 0){
        Radiant::BinaryData bd;
        MultiWidgets::MessageSendOperator * msg = new MultiWidgets::MessageSendOperator(
            2.0f, "reset-vacuum", bd);
        addOperator(msg);
    }
}

void VacuumReachingWidget::modifyVacuumWidget(long fingerId, double rotation, float intensity) {
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

void VacuumReachingWidget::processMessage(const char* msg, Radiant::BinaryData& bd)
{
    std::string s(msg);
    if (Radiant::StringUtils::beginsWith(s, "reset-vacuum")) {
        resetVectorField();
    }
    else {
        MultiWidgets::Widget::processMessage(msg, bd);
    }
}

void VacuumReachingWidget::update(float dt)
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

    //ensureWidgetsHaveBodies();
    //ensureGroundInitialized();

    //applyForceToBodies(dt);
    applyVacuum();

    if (m_featureFlags & FEATURE_GRAVITY)
        m_world.SetGravity(b2Vec2(m_gravity.x(), m_gravity.y()));

    for (int i=0; i < 1; ++i) {
        m_world.Step(dt/1, 10, 10);
    }
    m_world.ClearForces();

    updateBodiesToWidgets();
}

void VacuumReachingWidget::decayVectorField(Radiant::MemGrid32f (&field)[2], float dt) {
    float move = 0.1;
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

void VacuumReachingWidget::input(MultiWidgets::GrabManager & gm, float dt)
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
            //m_world.DestroyBody(m_bodies[hit] );
            //m_bodies.erase(m_bodies.find(hit));
			hit->touch();
            //hit->setDepth(0);

			/*			
			MultiWidgets::Widget * staticList = m_moving_to_static[hit];
			m_static_to_moving.erase(staticList);
			m_moving_to_static.erase(hit);
			deleteChild(staticList);
			*/

            //hit->recursiveSetAlpha(1.0f);
            //parent()->addChild(hit);
            std::cout << "hit!" << std::endl;
            hit->input(gm, dt);
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
        if (hit && hit == hit2 && strcmp(hit2->type(), "VacuumWidget") != 0) {
			if(m_bodies.count(hit) > 0)
			{
				m_world.DestroyBody(m_bodies[hit] );
				m_bodies.erase(m_bodies.find(hit));
			}
			hit->touch();
			
//			if(m_moving_to_static.count(hit) > 0)
//			{
//				m_moving_to_static[hit]->hide();
//				MultiWidgets::Widget * staticList = m_moving_to_static[hit];
//				staticList->hide();
//				m_static_to_moving.erase(staticList);
//				m_moving_to_static.erase(hit);
//				//std::cout << "deleteChild" << std::endl;
//				deleteChild(staticList);
//                //hit->recursiveSetAlpha(1.0f);
//			}

            std::cout << "hit 2!" << std::endl;
            hit->input(gm, dt);
			
            //parent()->addChild(hit);
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

				// Vacuum uses constant velocity (normalized)

                //float moveX = (nDir.x * 100)/width() * m_input_pull_intensity;
                //float moveY = (nDir.y * 100)/height() * m_input_pull_intensity;
                float moveX = nDir.x * m_input_pull_intensity;
                float moveY = nDir.y * m_input_pull_intensity;

				//*p1 = Math::Clamp(*p1 + Math::Min(moveX, 0.2f), -1.0f, 1.0f);
				//*p2 = Math::Clamp(*p2 + Math::Min(moveY, 0.2f), -1.0f, 1.0f);
				*p1 += moveX;
				*p2 += moveY;

				p1++;
				p2++;
			}
		}

		// Draw vacuum widget if not yet drawn
        // UPDATE: Allow only one vacuum (breaks 2 player mode)
        if(m_vacuumWidgets.count(f.id()) == 0 && m_vacuumWidgets.size() == 0) {
            addVacuumWidget(f.id(), f.initTipLocation(), diff.angle());
		}
		else
		{
			//std::cout << "diff.length() " << Nimble::Math::Sqrt(diff.length()-20) << std::endl;
			modifyVacuumWidget(f.id(), diff.angle(), Nimble::Math::Sqrt(diff.length()-20)/50);
		}
	}
	gm.popTransform();
}

void VacuumReachingWidget::render(Luminous::RenderContext & r)
{
	const Luminous::MultiHead::Area * area;
	Luminous::GLResources::getThreadMultiHead(0, &area);
	const Nimble::Vector2f gfxLoc = area->graphicsLocation();

	//std::cout << "Graphics size: x: " << area->graphicsSize().x << " y: " << area->graphicsSize().y << std::endl;

    GLRESOURCE_ENSURE(GLData, gld, this, r.resources());

    Luminous::Texture2D* textures[2] = {&gld->m_tex[0].ref(), &gld->m_tex[1].ref()};
    Luminous::Texture2D & textureRead = gld->m_tex[2].ref();

    r.pushTransformRightMul(transform());
    glDisable(GL_TEXTURE_2D);

    //renderChildren(r);

    Nimble::Vector2i texSize(w, h);
    //std::cout << "w: " << w << " h: " << h << std::endl;
    //glViewport(0, 0, area->width()*2, area->height());

    // Vary luminance based on force
    /*
    for (int i=0; i < 2; ++i) {
        textures[i]->bind(GL_TEXTURE1 + i);
        textures[i]->loadBytes(GL_LUMINANCE32F_ARB,
                               w, h, m_vectorFields[i].data(),
                               Luminous::PixelFormat::luminanceFloat(),
                               false);

    }
    */

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
    glViewport(0, 0, area->width()*2, area->height());


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

    Luminous::Utils::glTexRect(size(), r.transform());
    //Luminous::Utils::glTexRect(area->graphicsSize(), r.transform() * Nimble::Matrix3::translate2D(gfxLoc.x, gfxLoc.y));

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

    /*
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
    */

    //glBegin(GL_LINES);
    // Draw circles for area effect
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

            glBegin(GL_TRIANGLE_FAN);

            glColor4f(1.0, 0.8, 0.8, 0.05);
            glVertex2f(id.x, id.y);
            //glColor4f(0.0, 0.0, 1.0, 0.9);
            for(int ang = 0; ang < 360; ang += 10){
                float rad = ang * 3.14159/180;
                glVertex2f(id.x + sin(rad) * 30.0f, id.y + cos(rad) * 30.0f);
            }
            glEnd();
        }
    }
    //glEnd();

    // Draw lines from clones to originals
    glBegin(GL_LINES);
    // draw arrows for vector field
    std::map<MultiWidgets::Widget *, MultiWidgets::Widget *>::iterator it;
    for(it = m_vacuumClones.begin(); it != m_vacuumClones.end(); ++it){
        MultiWidgets::Widget * origin = it->first;
        MultiWidgets::Widget * clone = it->second;
        Nimble::Vector2 start(origin->sceneGeometry().center());
        Nimble::Vector2 end(clone->sceneGeometry().center());
        glColor4f(0.0, 0.0, 1.0, 0.3);
        glVertex2fv(start.data());
        glColor4f(0.0, 0.0, 1.0, 0.9);
        glVertex2fv(end.data());
    }
    glEnd();



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

void VacuumReachingWidget::addMovingAndStaticWidgetPair(MultiWidgets::Widget* staticWidget, MultiWidgets::Widget* movingWidget)
{
    m_moving_to_static[movingWidget] = staticWidget;
    m_static_to_moving[staticWidget] = movingWidget;
}


