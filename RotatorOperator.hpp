#ifndef ROTATOROPERATOR_H
#define ROTATOROPERATOR_H


// :(
#define private public
#include <MultiWidgets/RotateTowardsHandsOperator.hpp>
#undef private

#include <MultiWidgets/Animators.hpp>
#include <MultiWidgets/SimpleSDLApplication.hpp>
#include <Nimble/Math.hpp>

class RotatorOperator : public MultiWidgets::RotateTowardsHandsOperator { 
public:
  RotatorOperator() : MultiWidgets::RotateTowardsHandsOperator() {}
protected:

  virtual void update(MultiWidgets::Widget *w, float dt)
  {
    MultiWidgets::RotateTowardsHandsOperator::update(w, dt);
  }

  virtual void input(MultiWidgets::Widget* w, MultiWidgets::GrabManager& gm, float dt) {
    //Radiant::info("input %p", w);
    RotateTowardsHandsOperator::input(w, gm, dt);
  }

  virtual void applyRotation(MultiWidgets::Widget *w, float angle) {
    using namespace Nimble;

    float wangle = w->sceneRotation();
    float delta = angle - wangle;  

    while(delta > Math::PI)
      delta -= Math::TWO_PI;
    while(delta < -Math::PI)
      delta += Math::TWO_PI;

    if(Math::Abs(delta) < Math::HALF_PI * 0.7)
      return;

    float target = angle - wangle + w->rotation();
		
		Nimble::Vector2 dirsum(0,0);
		float tmove; // tipmove
		int nf=0; // fingers
		int nh=0; // hands
		scanForFingersRecursive(w, 
				*MultiWidgets::SimpleSDLApplication::instance()->grabManager(),
				dirsum, tmove, nf, nh);

		Radiant::info("Automatic rotation for %s: %f -> %f = %f; %d fingers; %d hands",
				w->name().c_str(),
				w->rotation(),
				target,
				fmod(fabsf(target-w->rotation()), 2*Nimble::Math::PI),
				nf,
				nh
				);
		MultiWidgets::AnimatorRotation::newRotator(w, Vector2(0.5, 0.5f), target, 0.3f);
  }
};

#endif // ROTATOROPERATOR_H
