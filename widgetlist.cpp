#include <MultiWidgets/Widget.hpp>
#include <MultiWidgets/StayInsideParentOperator.hpp>
#include <MultiWidgets/RotateTowardsHandsOperator.hpp>
#include <MultiWidgets/LimitScaleOperator.hpp>
#include <MultiWidgets/TextBox.hpp>
#include <Nimble/Random.hpp>

#include "widgetlist.hpp"

WidgetList::WidgetList(MultiWidgets::Widget * parent) : MultiWidgets::Widget(parent),
	m_type("WidgetList")
	{
	setName("WidgetList");
	setCSSType("WidgetList");
  setInputFlags(inputFlags() & ~INPUT_PASS_TO_CHILDREN);
}

WidgetList * WidgetList::clone(){
	WidgetList * list = WidgetList::createNiceList(parent());
	list->setStyle(style());
	list->setLocation(mapToParent(Nimble::Vector2(0, 0)));
	
	for (ItemList::iterator it = m_itemList.begin(); it != m_itemList.end(); ++it ) {
		MultiWidgets::TextBox * tb = dynamic_cast<MultiWidgets::TextBox*>(*it);
		MultiWidgets::TextBox * tb2 = new MultiWidgets::TextBox(0, 0, MultiWidgets::TextBox::HCENTER);
		tb2->setCSSClass("FloatingWord");
		tb2->setStyle(tb->style());
		tb2->setText(tb->text());
		tb2->setWidth(tb->width());
		tb2->setHeight(tb->height());
		tb2->setAlignFlags(MultiWidgets::TextBox::HCENTER | MultiWidgets::TextBox::VCENTER);
		list->addItem(tb2);
	}

	//layout();
	list->layout();
	list->setDepth(depth());
	list->setScale(scale());
	list->setRotation(rotation());

	return list;	
}

WidgetList * WidgetList::createNiceList(Widget * parent, Widget * content) {
    WidgetList * list = new WidgetList(parent);
    if (content)
      list->addItem(content);
    list->setDepth(-1);
    list->raiseFlag(WidgetList::LOCK_DEPTH);
    list->addOperator(new MultiWidgets::StayInsideParentOperator);
    //list->addOperator(new MultiWidgets::RotateTowardsHandsOperator);
    list->addOperator(new MultiWidgets::LimitScaleOperator(MultiWidgets::LimitScaleOperator::COMPARE_SCALE,
			    1.0f, 2.0f));
    return list;
}

void WidgetList::update(float dt)
{
  layout();
  if (m_itemList.empty())
    raiseFlag(DELETE_ME);
  for (ItemList::iterator it = m_itemList.begin(); it != m_itemList.end(); ++it ) {
    (**it).update(dt);
  }
  MultiWidgets::Widget::update(dt);
}

void WidgetList::layout()
{
  float offset = 0.0f;
  float ymax = 5.0f;
  for (ItemList::iterator it = m_itemList.begin(); it != m_itemList.end(); ++it ) {
    (*it)->setLocation(offset, 0);
    ymax = Nimble::Math::Max((*it)->height(), ymax);
    offset += (*it)->width();
  }
  setSize(offset, ymax);
}

void WidgetList::processFingers(MultiWidgets::GrabManager &gm, MultiWidgets::FingerArray &fingers, float dt)
{

  if (!parent()) return;


  // go through all the (MyList-)siblings that don't intersect with this already
  // and see if processing input makes us collide
  std::set<WidgetList*> plausible;
  for (ChildIterator it = parent()->childBegin(); it != parent()->childEnd(); ++it) {
    WidgetList * sibling = dynamic_cast<WidgetList*>(*it);
    if (!sibling || sibling == this)
      continue;

    if (!sibling->intersects(*this) && sibling->lastInteraction().sinceSecondsD() < 2.0) {
      //Radiant::info("%lf", sibling->lastInteraction().sinceSecondsD());
      plausible.insert(sibling);
    }
  }

  MultiWidgets::Widget::processFingers(gm, fingers, dt);

  // check for new collision between last item <-> first item
  for (std::set<WidgetList*>::iterator it = plausible.begin();
	it != plausible.end(); ++it) {
    if ((*it)->intersects(*this)) {
      WidgetList * ml = *it;
      assert(ml != 0);

      if (m_itemList.empty() || ml->m_itemList.empty())
	continue;

      // if last item collides with first item of this, put other list in the beginning
      // of this list.
      // otherwise, if first item collides with last item of this, append other list to this

      Nimble::Rectangle myFront = itemRect(m_itemList.front(), -1);
      Nimble::Rectangle myBack = itemRect(m_itemList.back(), 1);
      Nimble::Rectangle otherFront = ml->itemRect(ml->m_itemList.front(), -1);
      Nimble::Rectangle otherBack = ml->itemRect(ml->m_itemList.back(), 1);

      if (myBack.intersects(otherFront)) {
	m_itemList.splice(m_itemList.end(), ml->m_itemList);
	ml->layout();
	layout();
      } else if (myFront.intersects(otherBack)) {
	float offset = ml->width();
	setLocation(mapToParent(Nimble::Vector2(-offset, 0)));
	m_itemList.splice(m_itemList.begin(), ml->m_itemList);

	layout();
	ml->layout();
      }
    }
  }

  handleSplits(gm, fingers, dt);
}

void WidgetList::handleSplits(MultiWidgets::GrabManager &gm, MultiWidgets::FingerArray &fingers, float dt) {

  if (fingers.size() < 2)
    return;

  std::map<float, MultiTouch::Finger> fings;
  // Go through all the fingers touching this widget by their x-coordinates
  // (in local coordinates)
  // If two adjacent fingers have moved the opposites ways and are located in
  // adjacent widgets, split at that position

  for (unsigned int i=0; i < fingers.size(); ++i) {
    fings.insert(std::make_pair(gm.project(fingers[i].tipLocation()).x, fingers[i]));
  }

  std::map<float, MultiTouch::Finger>::iterator prev = fings.begin();
  std::map<float, MultiTouch::Finger>::iterator it = prev;
  ++it;

  for( ; it != fings.end(); ++it) {
    MultiTouch::Finger & c1 = prev->second;
    MultiTouch::Finger & c2 = it->second;
    Nimble::Vector2 mot1 = gm.project(c1.tipLocation())-gm.project(c1.prevTipLocation());
    Nimble::Vector2 mot2 = gm.project(c2.tipLocation())-gm.project(c2.prevTipLocation());
    mot1 /= dt; mot2 /= dt;
    // two fingers; left one moved left and right one moved right
    // (mot1.x can be zero only because of testing with mouse emulation..)
    if ((mot1.x < -250.0 && mot2.x > 250.0) || mot1.x < -500.0 || mot2.x > 500.0) {
      ItemList::iterator i1 = findItem(gm.project(c1.tipLocation()));
      ItemList::iterator i2 = findItem(gm.project(c2.tipLocation()));
      if (i1 != m_itemList.end() && i2 != m_itemList.end()) {
	ItemList::iterator i1_5 = i1;
	++i1_5;
	// check that widgets at finger positions are neighbours
	if (i1 != i2 && i1_5 == i2) {
	  WidgetList * list = WidgetList::createNiceList(parent());
	  list->setStyle(*style());
	  list->setLocation(mapToParent((**i2).mapToParent(Nimble::Vector2(0, 0)) + Nimble::Vector2(30, 0)));
	  list->m_itemList.splice(list->m_itemList.begin(), m_itemList, i2, m_itemList.end());

	  layout();
	  list->layout();
	  list->setDepth(depth());
	  list->setScale(scale());
	  list->setRotation(rotation());
	  Nimble::Vector2 vel(1000, Nimble::RandomUniform::instance().randMinMax(-200, 200));
	  vel.rotate(rotation());
	  dropAllGrabs(gm);
	  setHasInteraction(false);
	  /// @todo this has no effect, do something to force
	  setVelocity(-vel);
	  list->setVelocity(vel);
	  list->touch();
	}
      }
    }
  }
}

// return the bounding rectangle of widget in list in (common) parent coordinates
/// @param part -1 means left half, 0 full size, 1 right half
Nimble::Rectangle WidgetList::itemRect(const Widget * w, int part) {
  Nimble::Matrix3 m = transform();
  Nimble::Vector2 sz = w->size();
  if (part != 0)
    sz.x /= 2;

  m *= w->transform();
  m *= Nimble::Matrix3::translate2D(0.5f * sz);
  if (part > 0)
    m *= Nimble::Matrix3::translate2D(0.5f*w->width(), 0);
  return Nimble::Rectangle(sz, m);
}


// find item from list which contains local coordinate pos
WidgetList::ItemList::iterator WidgetList::findItem(Nimble::Vector2 pos)
{
  for (ItemList::iterator it = m_itemList.begin(); it != m_itemList.end(); ++it ) {
    if ((**it).isInside((**it).transform().inverse().project(pos)))
      return it;
  }
  return m_itemList.end();
}

void WidgetList::addItem(Widget * w) {
  m_itemList.push_back(w);
}

MultiWidgets::Widget* WidgetList::getItem(size_t idx) {
  ItemList::iterator it = m_itemList.begin();
  std::advance(it, idx);
  return *it;
}

size_t WidgetList::itemCount() const {
  return m_itemList.size();
}

void WidgetList::renderContent(Luminous::RenderContext &r)
{
  for (ItemList::iterator it = m_itemList.begin(); it != m_itemList.end(); ++it ) {
    (**it).render(r);
  }
}

const char * WidgetList::type() const
  {
	return m_type.c_str();
  }
