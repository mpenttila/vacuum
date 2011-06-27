#include <MultiWidgets/ImageWidget.hpp>

#include <MultiWidgets/StayInsideParentOperator.hpp>
#include <MultiWidgets/LimitScaleOperator.hpp>
#include <MultiWidgets/Plugins.hpp>
#include <MultiWidgets/TextBox.hpp>

#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Nimble/Random.hpp>
#include <RotatorOperator.h>

//#define USE_THREADED

#ifdef USE_THREADED
#include <ThreadedRendering/SimpleThreadedApplication.hpp>
typedef ThreadedRendering::SimpleThreadedApplication GfxApp;

#else
#include <MultiWidgets/SimpleSDLApplication.hpp>
typedef MultiWidgets::SimpleSDLApplication GfxApp;
#endif

#include "distortwidget.h"
#include "widgetlist.h"
#include "VacuumWidget.hpp"

#include <vector>
namespace {
	bool decomposeTransformation(Nimble::Matrix3& transform,
			Nimble::Vector2& translation,
			float& rotation, 
			Nimble::Vector2& scale)
	{
     scale.x = sqrt(transform[0][0]*transform[0][0] + transform[1][0]*transform[1][0]);
     scale.y = sqrt(transform[1][0]*transform[0][0] + transform[0][1]*transform[0][1]);
     rotation = atan2(-transform[0][1], transform[1][1]);

     translation.x = transform[0][2];
		 translation.y = transform[1][2];
		 return true;
	}
	void moveWidgetPreservingTransformation(MultiWidgets::Widget* w, MultiWidgets::Widget* to) {
		Nimble::Matrix3 m = to->sceneTransform().inverse() * w->sceneTransform();
		Nimble::Vector2 location, scale;
		float rotation;
	  decomposeTransformation(m, location, rotation, scale);
		to->addChild(w);
		w->setLocation(location);
		w->setRotation(rotation);
		// assuming uniform scaling
		w->setScale(scale.x);
	}

}

template <class T>
    void callWidgetTree(MultiWidgets::Widget * parent, T fn)
{
  fn(parent);
  MultiWidgets::Widget::ChildIterator it = parent->childBegin();
  MultiWidgets::Widget::ChildIterator end = parent->childEnd();
  while (it != end) {
    callWidgetTree(*it++, fn);
  }
}

class MyApplication : public GfxApp
{
  typedef void (*WidgetCallBack)(MultiWidgets::Widget*);
  typedef void (*CallBack)(void);
  typedef GfxApp Parent;
public:
  static MyApplication * me;
  MyApplication() : Parent(),
  m_automaticRotation(false),
  m_manualRotation(false)
  {
    assert(!me);
    me = this;
  }    

  void update()
  {
    std::list<CallBack>::iterator it;
    //    std::for_each(m_preUpdate.begin(), m_preUpdate.end(), boost::apply<void>());
    for (it = m_preUpdate.begin(); it != m_preUpdate.end(); ++it) {
      (*it)();
    }
    Parent::update();

    //   std::for_each(m_postUpdate.begin(), m_postUpdate.end(), boost::apply<void>());
    for (it = m_postUpdate.begin(); it != m_postUpdate.end(); ++it) {
      (*it)();
    }
  }
  virtual bool keyPressEvent (int ascii, int special, int modifiers) {
    //Radiant::info("Keypress: %d (%c)", ascii, ascii);
    if (ascii == 's') {
      int width = size().x;
      int height = size().y;
      Luminous::Image img;
      img.allocate(width, height, Luminous::PixelFormat(Luminous::PixelFormat::LAYOUT_RGB, Luminous::PixelFormat::TYPE_UBYTE));
      unsigned char *pb = img.bytes();
      assert(pb);
      glFlush();
      glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pb);
      img.flipVertical();
      char fname[64];
      time_t t;
      struct tm *time_now;
      t = time(NULL);
      time_now = localtime(&t);
      strftime(fname, 64, "screenshot-%Y%m%d-%H%M%S.tga", time_now);
      img.write(fname);
      return true;
    }
    else {
      return GfxApp::keyPressEvent(ascii, special, modifiers);
    }
  }
  
  MultiWidgets::Widget * overlay() { return m_overlay; }
  bool automaticRotation() { return m_automaticRotation; }
  bool manualRotation() { return m_manualRotation; }

  MultiWidgets::Widget * m_overlay;
  std::list<CallBack> m_preUpdate;
  std::list<CallBack> m_postUpdate;

  bool m_automaticRotation;
  bool m_manualRotation;
};
MyApplication * MyApplication::me = 0;

void backgroundify(MultiWidgets::Widget * w)
{
  MultiWidgets::Widget * d = MyApplication::me->overlay();
  float IDLE_TIMEOUT = d->getValue("idle-timeout")->asFloat();
  float IDLE_OK = d->getValue("idle-delay")->asFloat();
  float BG_ALPHA = d->getValue("background-alpha")->asFloat();
  if (w != MyApplication::me->overlay() && w->parent() == MyApplication::me->root()) {
    float idle = w->lastInteraction().sinceSecondsD();
    if (idle > IDLE_TIMEOUT && !w->inputTransparent()) {
      w->setColor(1, 1, 1, 1);
      MyApplication::me->overlay()->addChild(w);
    } else {
      float t = 0.0f;
      if (idle > IDLE_OK)
        t = (idle-IDLE_OK) / (IDLE_TIMEOUT-IDLE_OK);
      float a = Nimble::Math::Clamp(t*BG_ALPHA + (1-t)*1, 0.0f, 1.0f);
      w->setColor(1, 1, 1, a);
    }
  }
}

void check_rotations(MultiWidgets::Widget * w)
{
	// value is (old rotation, time since previous check)
	static std::map<MultiWidgets::Widget*, std::pair<float, int> > counters;
	static int check_interval = 100; // ~1 second
	if (!counters.count(w)) {		
		counters[w].second = 0;
		counters[w].first = w->rotation();
	}
	if (!w->hasInteraction()) {
		counters.erase(counters.find(w));
		return;
	}

	if (++counters[w].second == check_interval) {
		float r1 = w->rotation();
		float r2 = counters[w].first;
		float deltarot = fmod(fabsf(r2-r1), 2*Nimble::Math::PI);

		float threshold = 0.7*Nimble::Math::PI/2;
		if (deltarot > threshold) {
		Radiant::info("Manual rotation for %s; %f -> %f = %f; fingers %d; hands %d",
				w->name().c_str(),
				r1, r2,
				deltarot,
				w->grabFingerCount(),
				w->grabHandCount()
				);
		}
		counters[w].second = 0;
		counters[w].first = r1;
	}

}

void bg()
{
  callWidgetTree(MyApplication::me->root(), backgroundify);
	if (MyApplication::me->manualRotation()) {
    callWidgetTree(MyApplication::me->root(), check_rotations);
	}
}

class AnswerBoard : public MultiWidgets::Widget {
  typedef MultiWidgets::Widget Parent;
public:	
  AnswerBoard(MultiWidgets::Widget *p=0) : Parent(p)
  {
		setName("AnswerBoard");
    setCSSType("AnswerBoard");
    //setMomentum(0);
  }

  int answerCount() {
    int count=0;
    MultiWidgets::Widget::ChildIterator it;
    for (it = childBegin(); it != childEnd(); ++it) {
      if (dynamic_cast<WidgetList*>(*it))
        ++count;
    }
    return count;
  }

  void processFingers(MultiWidgets::GrabManager &gm, MultiWidgets::FingerArray &fingers, float dt)
  {
    Parent::processFingers(gm, fingers, dt);
#if 0
//    setLocation(m_fixedCenterLocation);

    setLocation(0, 0);
    Nimble::Matrix3 m = parent()->transform() * transform();

    Nimble::Vector2 tr, sc;
    float rot;

    decomposeTransformation(m, tr, rot, sc);

    parent()->setLocation(tr);
    parent()->setRotation(rot);
    parent()->setScale(sc.x);
    //parent()->setRotation(parent()->rotation() + rotation());

    setScale(1);
    setLocation(10, 10);
    setRotation(0);
#endif
  }

  void update(float dt)
  {
    if (!parent() || !parent()->parent())
      return;

    MultiWidgets::Widget * top = parent()->parent();


    MultiWidgets::Widget::ChildIterator it;
		// after this many seconds without interaction, intersecting
		// widgets no longer stick to this
		float maxIdle = 1.0f;

    for (it = childBegin(); it != childEnd(); ++it) {
      if (!it->hasInteraction() && !it->intersects(*this)) {
				moveWidgetPreservingTransformation(*it, top);
      }
    }

    for (it = top->childBegin(); it != top->childEnd(); ++it) {
      if (it->lastInteraction() > 0 &&
					!it->hasInteraction() &&
					it->lastInteraction().sinceSecondsD() < maxIdle &&
					it->intersects(*this)) {
        if (dynamic_cast<WidgetList*>(*it)) {
					moveWidgetPreservingTransformation(*it, this);
        }
      }
    }
    Parent::update(dt);    
  }

  Nimble::Vector2f m_fixedCenterLocation;
};


class WordPreviewWidget : public MultiWidgets::Widget {
typedef MultiWidgets::Widget Parent;
public:
  WordPreviewWidget(MultiWidgets::Widget* p=0)
		: Parent(p),
		m_sentenceBox(new MultiWidgets::TextBox(this)),
    m_submit(new MultiWidgets::TextBox(this))
	{
		setName("WordPreview");
    /*
    setInputFlags(INPUT_USE_SINGLE_TAPS | INPUT_PASS_TO_CHILDREN);
		if (MyApplication::me->manualRotation()) {
			setInputFlags(inputFlags() | INPUT_ROTATION);
		}
    */
/*
		if (MyApplication::me->automaticRotation()) {
			addOperator(new RotatorOperator);
		}
*/		
		m_submit->setInputFlags(MultiWidgets::Widget::INPUT_USE_SINGLE_TAPS);
		m_submit->setAlignFlags(MultiWidgets::TextBox::HCENTER |
				MultiWidgets::TextBox::VCENTER);
		m_submit->setText("Submit!");
		m_sentenceBox->setFixed(true);

    m_sentenceBox->setSize(Nimble::Vector2(400, 120));
		m_sentenceBox->setFaceSize(20);    
		m_submit->setLocation(Nimble::Vector2(0, 120));
		m_submit->setSize(Nimble::Vector2(400, 30));
		m_submit->setPadding(0); m_sentenceBox->setPadding(0);
		m_submit->setBorder(0); m_sentenceBox->setBorder(0);
		setPadding(0);

    setSize(400, 150);

    setAutoBringToTop(false);
  	m_submit->setAutoBringToTop(false);
  	m_sentenceBox->setAutoBringToTop(false);
	}

	virtual void update(float dt)
	{

    setCenterLocation(m_fixedCenterLocation);
		Radiant::Color c = m_submit->color();
		if (m_submit->isVisible() && m_submit->hasInteraction()) {
			float since = m_submit->interactionBegan().sinceSecondsD() / 2.0f;			
			c[3] = 1-since;
			if (since > 1 && m_lastEvent.sinceSecondsD() > 3.0f) {
				m_lastEvent = Radiant::TimeStamp::getTime();
				eventSend("player-submit");
			  m_submit->dropAllGrabs(*MultiWidgets::SimpleSDLApplication::instance()->grabManager());
			}

		} else {
			c[3] = 1.0f;
		}
		m_submit->setColor(c);

    Parent::update(dt);
	}

	// workaround
  Nimble::Vector2f m_fixedCenterLocation;
	Radiant::TimeStamp m_lastEvent;

	MultiWidgets::TextBox* m_sentenceBox;
	MultiWidgets::TextBox* m_submit;
};

/*
class WordGameUserWidget : public MultiWidgets::Widget {
	typedef MultiWidgets::Widget Parent;
	public:
	WordGameUserWidget(MultiWidgets::Widget* p=0)
		: Parent(p),
		m_answerBoard(new AnswerBoard(this)),
		m_preview(new WordPreviewWidget(this))
	{

	}

	AnswerBoard* m_answerBoard;
	WordPreviewWidget* m_preview;
};
*/

class WordGameWidget : public MultiWidgets::Widget {
	typedef MultiWidgets::Widget Parent;
  typedef std::vector<std::string> Line;

public:

  AnswerBoard* getAnswerBoard(size_t idx) {
		return m_answerBoards[idx];
	}

  WordPreviewWidget* getPreview(size_t idx) {
		return m_previews[idx];
	}

	WordGameWidget(int players, MultiWidgets::Widget * p=0) : 
		Parent(p),
		m_current(-1),
		m_players(players),
    m_duplicateCount(this, "duplicate-words", 2),
		m_playersPassed(0)

	{
		setName("WordGame");
    setCSSType("WordGame");
    setAutoBringToTop(false);

		m_answerBoards.resize(players);
		m_previews.resize(players);

		Nimble::Vector2 sz = MyApplication::me->size();
		int previewH = 150;
		int previewW = 400;
		Nimble::Vector2 previewLocs[] = {
			Nimble::Vector2(0, sz.y/2 - previewH),
			Nimble::Vector2(sz.x-previewW, sz.y/2 - previewH)
		};
		const char* submitClasses[] = {
			"submit_1", "submit_2"
		};
		const char * answerClasses[] = {
			"answer_1", "answer_2"
		};

		for (int i=0; i < players; ++i) {
			m_previews[i] = new WordPreviewWidget(this);

      //Widget* answerFrame = new Widget(this);
      m_answerBoards[i] = new AnswerBoard(this);

      //answerFrame->setInputFlags(INPUT_MOTION_XY | INPUT_PASS_TO_CHILDREN);

      WordPreviewWidget* preview = getPreview(i);

			std::string eventName = std::string("submit-button-pressed-") + Radiant::StringUtils::stringify(i);
      preview->eventAddListener("player-submit", eventName.c_str(), this);
      preview->setLocation(previewLocs[i]);
      preview->setRotationAboutCenter(-Nimble::Math::PI/2 + i*Nimble::Math::PI);
      preview->m_submit->setCSSClass(submitClasses[i]);
      preview->m_fixedCenterLocation = preview->mapToParent(preview->size()*0.5f);
			m_answerBoards[i]->setCSSClass(answerClasses[i]);
			m_answerBoards[i]->setColor(255, 0, 0, 255);			
      m_answerBoards[i]->setSize(300, 300);
			m_answerBoards[i]->addOperator(new MultiWidgets::StayInsideParentOperator(1000,
						MultiWidgets::StayInsideParentOperator::DEFAULT_FLAGS, 1));


      /*
      answerFrame->setLocation(previewLocs[i].x, previewLocs[i].y + 150 + 20);
      */
      //answerFrame->setSize(300+20, 300+20);
      m_answerBoards[i]->setLocation(previewLocs[i].x, previewLocs[i].y + 150);
      //m_answerBoards[i]->m_fixedCenterLocation = m_answerBoards[i]->location();
			//m_answerBoard->setRotationAboutCenter(Nimble::Math::PI/2);
			m_answerBoards[i]->setAutoBringToTop(false);
			m_answerBoards[i]->setInputFlags(INPUT_MOTION_XY | INPUT_ROTATION | INPUT_PASS_TO_CHILDREN);

      preview->setInputFlags(INPUT_MOTION_XY | INPUT_ROTATION | INPUT_PASS_TO_CHILDREN);
			if (!MyApplication::me->manualRotation()) {
				m_answerBoards[i]->setInputFlags(m_answerBoards[i]->inputFlags() & ~INPUT_ROTATION);
        preview->setInputFlags(preview->inputFlags() & ~INPUT_ROTATION);
			}
		  if (MyApplication::me->automaticRotation()) {
        m_answerBoards[i]->addOperator(new RotatorOperator);
        preview->addOperator(new RotatorOperator);
  		}
		}
/*
		if (MyApplication::me->automaticRotation()) {
      addOperator(new RotatorOperator);
		}
*/
		
		setInputFlags(MultiWidgets::Widget::INPUT_PASS_TO_CHILDREN);

    //setRotation(-Nimble::Math::PI/2);
    //setRotationAboutCenter(-Nimble::Math::PI/2);
		//
	}
	
	virtual void update(float dt) {
    Parent::update(dt);
	}

  static bool is_special(char c) {
    return !std::isalnum(c);
  }

  static bool cmp_string_ascii_only(std::string s1, std::string s2)
  {
    std::string::iterator i1 = std::remove_if(s1.begin(), s1.end(), is_special);
    std::string::iterator i2 = std::remove_if(s2.begin(), s2.end(), is_special);
    s1.erase(i1, s1.end());
    s2.erase(i2, s2.end());
		// lowercase compare 
		std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
		std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
    return s1 == s2;
  }
	
	virtual void processMessage(const char* msg, Radiant::BinaryData& bd)
	{
    std::string s(msg);

		if (Radiant::StringUtils::beginsWith(s, "submit-button-pressed")) {
			// must take at least 3 seconds
			if (m_levelStartedAt.sinceSecondsD() < 3.0f) {
				return;
			}

			// go through all sentences
			int idx = s[s.size()-1] - '0';
			AnswerBoard* answerBoard = m_answerBoards[idx];

      DistortWidget* dw = dynamic_cast<DistortWidget*>(MyApplication::me->overlay());
      assert(dw != 0);
      dw->removeStaticObject(m_previews[idx]);

      std::vector<bool> found(m_lines[m_current].size(), false);

      // go through all the word lists in answer board, see if they match something
      for (MultiWidgets::Widget::ChildIterator it = answerBoard->childBegin();
      it != answerBoard->childEnd(); ++it) {
        WidgetList * wl = dynamic_cast<WidgetList*>(*it);
        if (!wl)
          continue;

        std::string txt;
        for (int i=0; i < wl->itemCount(); ++i) {
          MultiWidgets::TextBox * tb = dynamic_cast<MultiWidgets::TextBox*>(wl->getItem(i));
          txt += Radiant::StringUtils::stdWstringToStdString(tb->text()) + ' ';
        }

        Radiant::info("Player %d answer: %s", idx, txt.c_str());

        std::vector<Line>& lines = m_lines[m_current];

        for (int i=0; i < lines.size(); ++i) {
          if (found[i])
            continue;

          Line& line = lines[i];

          if (wl->itemCount() != line.size())
            continue;

          int j;
          for (j=0; j < line.size(); ++j) {
            MultiWidgets::TextBox * tb = dynamic_cast<MultiWidgets::TextBox*>(wl->getItem(j));
            if (!tb)
              continue;

            std::string txt = Radiant::StringUtils::stdWstringToStdString(tb->text());
            if (!cmp_string_ascii_only(txt, line[j]))
              break;
          }
          if (j == line.size())
            found[i] = true;
        }
      }



      int correct=0;
      for (int i=0; i < found.size(); ++i) {       
        if (found[i])
          ++correct;
      }

      if (true) {
        Radiant::info("Level %d passed for player %d: took %lf seconds; %d/%d correct; %d items on answer board",
						m_current,
						idx,
						m_levelStartedAt.sinceSecondsD(),
						correct, m_lines[m_current].size(),
						answerBoard->answerCount()
						);
				m_playersPassed++;
				///@todo sprinkle words from answerboard around?
				m_answerBoards[idx]->setIsVisible(false);
				m_previews[idx]->setIsVisible(false);
				if (m_playersPassed == m_players) {

          gotoLevel(m_current + 1);
				}
      }
		} else {
			Parent::processMessage(msg, bd);
		}
	}

	/// @param baseFilename words will be loaded from {baseFilename}.words and
	///        expected sentence from {baseFilename}.expected
  void addLevel(const std::string& baseFilename) {
		
		std::ifstream words( (baseFilename + ".words").c_str() );
    std::string word;
		m_wordLists.resize(m_wordLists.size()+1);
    std::vector<std::string>& wordList = m_wordLists.back();

		while (words >> word) {
			wordList.push_back(word);
		}

		std::ifstream expected( (baseFilename + ".expected").c_str() );
		m_sentences.resize(m_sentences.size() + 1);
    std::vector<std::string>& sentence = m_sentences.back();
    m_lines.resize(m_lines.size() + 1);
    std::vector<Line>& lines = m_lines.back();

    std::string line;
    while (std::getline(expected, line)) {
      std::stringstream ss;
      ss << line;
      lines.resize(lines.size()+1);
      while (ss >> word) {
        sentence.push_back(word);
        lines.back().push_back(word);
      }
    }

    /*
		while (expected >> word) {
			sentence.push_back(word);
    }
    */
	}
	
	void gotoLevel(int level) {    
    Nimble::RandomUniform rnd(55);
		m_playersPassed = 0;
		// delete everything, create again

		float maxH = 10;
		MyApplication& app = *MyApplication::me;
    bool automaticRot = app.automaticRotation();
    bool manualRot = app.manualRotation();

    MultiWidgets::Widget::ChildIterator it;
    for (it = app.root()->childBegin(); it != app.root()->childEnd(); ++it) {
      if (dynamic_cast<WidgetList*>(*it))
        app.root()->deleteChild(*it);
    }
    for (it = app.overlay()->childBegin(); it != app.overlay()->childEnd(); ++it) {
      if (dynamic_cast<WidgetList*>(*it))
        app.overlay()->deleteChild(*it);
    }
		for (int i=0; i < m_players; ++i) {
      getAnswerBoard(i)->deleteChildren();
		}

    // delete static objects just in case
    DistortWidget* dw = dynamic_cast<DistortWidget*>(app.overlay());
    assert(dw != 0);
    for (int i=0; i < m_players; ++i)
      dw->removeStaticObject(getPreview(i));

    for (int i=0; i < m_players; ++i) {
      MultiWidgets::Widget* w = getPreview(i);
      Nimble::Vector2 center = w->mapToScene(w->size()*0.5f);
      Nimble::Vector2 topLeft = w->mapToScene(Nimble::Vector2(0,0));
      float rad = (center-topLeft).length();

      dw->addStaticDisk(w, center, rad * 0.95f);
    }


    if (level >= m_wordLists.size())
      return;

	m_current = level;
    std::vector<std::string>& words = m_wordLists[m_current];
	for (int i=0; i < m_players; ++i) {
      getAnswerBoard(i)->setIsVisible(true);
      getPreview(i)->setIsVisible(true);
	}

	int yval = 20;
	int xval = app.size().x * 0.2f;
	for (int i=0; i < words.size(); ++i){
		for (int j=0; j < m_duplicateCount; ++j) {
			
			if(xval > (app.size().x * 0.8f)){
				xval = app.size().x * 0.2f;
				yval += 50;
			}
			
			std::string& word = words[i];

			MultiWidgets::TextBox * tb = new MultiWidgets::TextBox(0, 0, MultiWidgets::TextBox::HCENTER);
			tb->setCSSClass("FloatingWord");

			tb->setStyle(app.style());
			tb->setText(word);
			tb->setWidth(tb->totalTextAdvance() + tb->padding()*2 + 10);
			tb->setHeight(1.5 * tb->heightForWidth(tb->width()));
			tb->setAlignFlags(MultiWidgets::TextBox::HCENTER |
					MultiWidgets::TextBox::VCENTER);
			maxH = Nimble::Math::Max(maxH, tb->height());

			WidgetList * list = WidgetList::createNiceList(app.root(), tb);
			if(automaticRot) {
			  list->addOperator(new RotatorOperator);
			}
			if (!manualRot) {
			  list->setInputFlags(list->inputFlags() & ~INPUT_ROTATION);
			}
			list->setStyle(app.style());
			list->setLocation(Nimble::Vector2(xval, yval));
			//list->setRotation(rnd.rand01()*2*3.145926);
			list->raiseFlag(WidgetList::LOCK_DEPTH);
			
			xval += 150;
		}
	}
    std::string w;

    /*
		for (int i=0; i < m_sentences[m_current].size(); ++i) {
			w += m_sentences[m_current][i] + ' ';
		}
    */

    for (int i=0; i < m_lines[m_current].size(); ++i) {
      Line& line = m_lines[m_current][i];
      for (int j=0; j < line.size(); ++j)
        w += line[j] + ' ';
      w += "\n";
    }


    Radiant::info("Starting level %d: poem:\n %s",
                  m_current, w.c_str());

		for (int i=0; i < m_players; ++i) {
      getPreview(i)->m_sentenceBox->setText(w);
		}

    m_levelStartedAt = Radiant::TimeStamp::getTime();
	}

  std::vector<std::vector<Line> > m_lines;
  std::vector<std::vector<std::string> > m_sentences;
  std::vector<std::vector<std::string> > m_wordLists;
	int m_current;

	std::vector<AnswerBoard*> m_answerBoards;
	std::vector<WordPreviewWidget*> m_previews;
	/*
	MultiWidgets::TextBox * m_sentenceBox;
	MultiWidgets::TextBox * m_submit;
  AnswerBoard * m_answerBoard;
	*/
  Radiant::TimeStamp m_levelStartedAt;

  Valuable::ValueInt m_duplicateCount;
	int m_players;
	int m_playersPassed;
};

int main(int argc, char ** argv)
{
#ifndef USE_THREADED
  SDL_Init(SDL_INIT_VIDEO);
#endif
  MyApplication app;
  app.m_postUpdate.push_back(bg);

  if(!app.simpleInit(argc, argv))
    return 1;

  const char * dirpath = "Images";

  uint32_t featureFlags = 0;
	int players = 1;
	int levelCount = 2;

  // Scan command line arguments for directory name
  for(int i = 0; i < argc; i++) {
    std::string r = argv[i];
    if (r == "dir" && (i+1) < argc) {
      dirpath = argv[i+1];
      ++i;
    } else if (r == "--arrows") {
      featureFlags |= DistortWidget::FEATURE_ARROWS;
    } else if (r == "--particles") {
      featureFlags |= DistortWidget::FEATURE_PARTICLES;
    } else if (r == "--manualrotation") {
      app.m_manualRotation = true;
    } else if (r == "--automaticrotation") {
      app.m_automaticRotation = true;
    } else if (r == "--players" && (i+1) < argc) {
			players = atoi(argv[++i]);
    } else if (r == "--levels" && (i+1) < argc) {
			levelCount = atoi(argv[++i]);
		}

  }

  DistortWidget * d = new DistortWidget(app.root());
  d->setFeatureFlags(featureFlags);
  d->setSize(app.root()->size());
  d->setStyle(app.style());
  //d->resize(d->width(), d->height());
  // every 32 pixels
  const int density = 32;
  d->resize(Nimble::Math::Round(d->width()/density), Nimble::Math::Round(d->height()/density));
  d->setDepth(-30);
  d->setInputTransparent(true);
  //MultiWidgets::Widget * root = d;
  app.m_overlay = d;
  // Open a directory with images:
  Radiant::Directory directory(dirpath, "jpeg,jpg,png");

  Nimble::RandomUniform rnd(5);


	WordGameWidget * wg = new WordGameWidget(players, app.root());
	wg->setDepth(-25);
	wg->raiseFlag(WordGameWidget::LOCK_DEPTH);
	wg->setAutoBringToTop(false);
//	wg->setLocation(50, 400);
//	d->addStaticBox(wg->sceneGeometry());
//  d->addOutsideWidget(wg, 500, 1.0f);

  std::vector<std::string> levels;
	for (int i=1; i <= 12; ++i) {
    levels.push_back(Radiant::StringUtils::stringify(i));
	}

  std::random_shuffle(levels.begin(), levels.end());

	bool rotation = true;
  float maxH = 10;
  {
    for (int i=0; i < levelCount; ++i) {
      wg->addLevel(levels[i]);
    }
    /*
		wg->addLevel("first");
    wg->addLevel("second");
    wg->addLevel("third");
    */
		wg->gotoLevel(0);
    wg->setStyle(app.style());
  }
  
/*  VacuumWidget * vac = new VacuumWidget();
  
      // Set widget parameters:
    vac->setThickness(50);
    vac->setSize(Nimble::Vector2(200, 200));
    vac->setLocation(Nimble::Vector2(600, 300));
    // Create random colors for the widgets:
    vac->setColor(Nimble::Vector4(0.4, 1.0f - 0.2, 1.0, 0.97));

    // Add the widget to the scene:
    wg->addChild(vac);
 */ 
  return app.run();
}
