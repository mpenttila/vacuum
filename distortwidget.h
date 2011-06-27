#ifndef DISTORTWIDGET_H
#define DISTORTWIDGET_H

#include <MultiWidgets/ImageWidget.hpp>
#include <Radiant/Grid.hpp>
#include <Luminous/Utils.hpp>
#include <Box2D/Box2D.h>

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
class DistortWidget : public MultiWidgets::Widget {
  // ensure appropriate scale for box2d

  static Nimble::Vector2 fromBox2D(const b2Vec2 & v) { return Nimble::Vector2(v.x, v.y)*BOX2D_SCALE; }
  static b2Vec2 toBox2D(const Nimble::Vector2 & v) { return b2Vec2(v.x/BOX2D_SCALE, v.y/BOX2D_SCALE); }
	static float toBox2D(float v) { return v/BOX2D_SCALE; }
public:
  enum FeatureFlags {
    FEATURE_ARROWS = 1 << 0,
    FEATURE_PARTICLES = 1 << 1,
    FEATURE_VELOCITY_FIELD = 1 << 2,
    FEATURE_GRAVITY = 1 << 3
  };

  DistortWidget(MultiWidgets::Widget * parent = 0);

  virtual void deleteChild(Widget *w);
  void setFeatureFlags(uint32_t flags);
  void resize(int width, int height);
  void update(float dt);
  void updateParticles(float dt);
  void decayVectorField(Radiant::MemGrid32f (&field)[2], float dt);
  void ensureWidgetsHaveBodies();
  void ensureGroundInitialized();
  void applyForceToBodies(float dt);
  void updateBodiesToWidgets();
  Nimble::Vector2 vectorValue(Nimble::Vector2 v);
  Nimble::Vector2 vectorOffset(Nimble::Vector2 pos);
  void blur(float ratio);
  Nimble::Vector2 mapInput(Nimble::Vector2 v);
  MultiWidgets::Widget * findChildInside(Luminous::Transformer & tr, Nimble::Vector2f loc, MultiWidgets::Widget * parent);
  void input(MultiWidgets::GrabManager & gm, float dt);
  void render(Luminous::RenderContext & r);

  void addStaticDisk(void* id, Nimble::Vector2 center, float rad);
  void removeStaticObject(void* id);
	void addStaticBox(const Nimble::Rectangle& rect);
	void addOutsideWidget(MultiWidgets::Widget * w, float density, float friction);

  int w, h;
  Radiant::MemGrid32f m_vectorFields[2];

  struct GLData;

  std::vector<Particle> m_particles;
  std::vector<int> m_free_particles;
  std::set<long> m_currentFingerIds;
  Valuable::ValueIntT<uint32_t> m_featureFlags;
//  uint32_t m_featureFlags;
  b2World m_world;
  std::map<void*, b2Body*> m_bodies;
  std::map<void*, b2Body*> m_statics;

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
};

#endif // DISTORTWIDGET_H
