#include <MultiWidgets/ImageWidget.hpp>

#include <MultiWidgets/StayInsideParentOperator.hpp>
#include <MultiWidgets/LimitScaleOperator.hpp>
#include <MultiWidgets/Plugins.hpp>
#include <MultiWidgets/TextBox.hpp>

#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Nimble/Random.hpp>
#include <RotatorOperator.hpp>

//#define USE_THREADED

#ifdef USE_THREADED
#include <ThreadedRendering/SimpleThreadedApplication.hpp>
typedef ThreadedRendering::SimpleThreadedApplication GfxApp;

#else
#include <MultiWidgets/SimpleSDLApplication.hpp>
typedef MultiWidgets::SimpleSDLApplication GfxApp;
#endif

#include "distortwidget.hpp"
#include "widgetlist.hpp"
#include "VacuumWidget.hpp"
#include "wordreader.hpp"
#include "RoundTextBox.hpp"

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
		m_manualRotation(true)
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

	DistortWidget * overlay() {
		return m_overlay;
	}
	bool automaticRotation() {
		return m_automaticRotation;
	}
	bool manualRotation() {
		return m_manualRotation;
	}

	DistortWidget * m_overlay;
	std::list<CallBack> m_preUpdate;
	std::list<CallBack> m_postUpdate;

	bool m_automaticRotation;
	bool m_manualRotation;
};
MyApplication * MyApplication::me = 0;

void backgroundify(MultiWidgets::Widget * w)
{
	DistortWidget * d = MyApplication::me->overlay();
	float IDLE_TIMEOUT = d->getValue("idle-timeout")->asFloat();
	float IDLE_OK = d->getValue("idle-delay")->asFloat();
	float BG_ALPHA = d->getValue("background-alpha")->asFloat();
	if (w != MyApplication::me->overlay() && w->parent() == MyApplication::me->root()) {
		float idle = w->lastInteraction().sinceSecondsD();
		if (idle > IDLE_TIMEOUT && !w->inputTransparent()) {
			w->setColor(1, 1, 1, 1);
			w->recursiveSetAlpha(0.0f);
			MyApplication::me->overlay()->addChild(w);
			/*
			WidgetList * list = dynamic_cast<WidgetList*>(w);
			WidgetList * clone = list->clone();
			*/
			RoundTextBox * tb = dynamic_cast<RoundTextBox*>(w);
			RoundTextBox * clone = tb->clone();
			clone->setType("clone");
			clone->setInputTransparent(true);
			d->m_static_to_moving[clone] = w;
			d->m_moving_to_static[w] = clone;
		} else {
			float t = 0.0f;
			if (idle > IDLE_OK)
				t = (idle-IDLE_OK) / (IDLE_TIMEOUT-IDLE_OK);
			float a = Nimble::Math::Clamp(t*BG_ALPHA + (1-t)*1, 0.0f, 1.0f);
			w->setColor(1, 1, 1, a);
		}
	}
}

void bg()
{
	callWidgetTree(MyApplication::me->root(), backgroundify);
}

class CollectedWordsWidget : public MultiWidgets::Widget{
	
public:
	
	CollectedWordsWidget(MultiWidgets::Widget * p=0): MultiWidgets::Widget(p), m_yloc(0), m_xloc(30)
	{
		setName("CollectedWordsWidget");
		setCSSType("CollectedWordsWidget");
	}
	
	void addWord(const std::string & word)
	{
		MultiWidgets::TextBox * tb = new MultiWidgets::TextBox(this, word.c_str(), MultiWidgets::TextBox::HCENTER | MultiWidgets::TextBox::VCENTER);
		tb->setCSSType("CollectedWord");
		tb->setStyle(style());
		tb->setInputTransparent(true);
		//tb->setRotation(-1 * Nimble::Math::HALF_PI);
		tb->setHeight(50);
		tb->setFaceSize(36);
		tb->setWidth(tb->totalTextAdvance() + 30);
		if((m_yloc + tb->totalTextAdvance() + 30) > width()){
			m_yloc = 0;
			m_xloc += 42;
		}
		tb->setLocation(m_yloc, m_xloc);
		m_yloc += tb->width();
	}
	
	int m_yloc;
	int m_xloc;
	
};

class WordGameWidget : public MultiWidgets::Widget {
	typedef MultiWidgets::Widget Parent;
	typedef std::vector<std::string> Line;

public:

	WordGameWidget(int players, MultiWidgets::Widget * p=0, DistortWidget * _d = 0, double physicalWidth = 0) :
		Parent(p),
		m_currentsentence(1),
		m_currentword(0),
		m_players(players),
		m_playersPassed(0),
		m_physicalWidth(physicalWidth),
		wordReader(players)
	{
		setName("WordGame");
		setCSSType("WordGame");
		setAutoBringToTop(false);

		m_collectedWords.resize(players);
		m_startButtons.resize(players);
		m_scoreWidgets.resize(players);
		for(int i = 0; i < 2; i++){
			m_playerReadyToStart[i] = false;
			m_playerScore[i] = 0;
		}

		d = _d;

		Nimble::Vector2 sz = MyApplication::me->size();
		int preH = 200;
		int preW = 800;
		Nimble::Vector2 previewLocs[] = {
			Nimble::Vector2(0, sz.y - preH),
			Nimble::Vector2(sz.x - preW, sz.y - preH)
		};

		for (int i=0; i < players; ++i) {
			m_collectedWords[i] = new CollectedWordsWidget(this);
			m_collectedWords[i]->setWidth(preW);
			m_collectedWords[i]->setHeight(preH);
			//m_collectedWords[i]->setRotationAboutCenter(-Nimble::Math::PI/2 + i*Nimble::Math::PI);
			m_collectedWords[i]->setLocation(previewLocs[i]);
			m_collectedWords[i]->setInputTransparent(true);
			
			std::string buttonName = std::string("P") + Radiant::StringUtils::stringify(i+1) + std::string(" Start");
			std::string buttonEvent = std::string("start-button-pressed-") + Radiant::StringUtils::stringify(i);
			m_startButtons[i] = new RoundTextBox(this, buttonName.c_str(), MultiWidgets::TextBox::VCENTER | MultiWidgets::TextBox::HCENTER);
			m_startButtons[i]->eventAddListener("interactionbegin", buttonEvent.c_str(), this);
			m_startButtons[i]->setCSSType("StartButton");
			m_startButtons[i]->setStyle(style());
			/* Set width to 20 mm */
			int startWidth = Nimble::Math::Round(sz.x / m_physicalWidth * 20);
			m_startButtons[i]->setWidth(startWidth);
			m_startButtons[i]->setLocation(sz.x/2 - startWidth - 50 + (100 + startWidth) * i, sz.y/2 - startWidth);
			m_startButtons[i]->setInputFlags(MultiWidgets::Widget::INPUT_USE_TAPS);
			m_startButtons[i]->setDepth(0);
			m_startButtons[i]->setIsVisible(false);
			
			m_scoreWidgets[i] = new MultiWidgets::TextBox(this, 0, MultiWidgets::TextBox::HCENTER);
			m_scoreWidgets[i]->setCSSType("ScoreWidget");
			m_scoreWidgets[i]->setStyle(style());
			m_scoreWidgets[i]->setText(std::string("0"));
			m_scoreWidgets[i]->setWidth(m_scoreWidgets[i]->totalTextAdvance() * 2 + 50);
			m_scoreWidgets[i]->setInputTransparent(true);
			m_scoreWidgets[i]->setColor(0, 0, 0, 0);
			m_scoreWidgets[i]->setLocation(m_startButtons[i]->location().x, 100);
			
		}

		setInputFlags(MultiWidgets::Widget::INPUT_PASS_TO_CHILDREN);
		
		eventAddListener("clear-widget", "clear-widget", this);
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
	
	void initializeLevel(){
		
		m_currentWordWidgets.clear();
		++m_currentword;

		if(m_currentword > wordReader.sentenceLength(m_currentsentence)){
			++m_currentsentence;
			if(m_currentsentence > wordReader.maxSentence()){
				// End
				return;
			}
			m_currentword = 1;
		}
		for(int i = 0; i < m_players; ++i){
			m_playerReadyToStart[i] = false;
			m_startButtons[i]->setIsVisible(true);
		}
		m_playersPassed = 0;
	}

	virtual void processMessage(const char* msg, Radiant::BinaryData& bd)
	{
		std::string s(msg);
		
		if (Radiant::StringUtils::beginsWith(s, "start-button-pressed")) {
			int player = s[s.size()-1] - '0';
			m_playerReadyToStart[player] = true;
			for(int i = 0; i < m_players; ++i){
				if(!m_playerReadyToStart[i]){
					return;
				}
			}
			for (int i=0; i < m_players; ++i) {
				m_startButtons[i]->setIsVisible(false);
			}
			gotoLevel(m_currentsentence, m_currentword);
		}
		else if(Radiant::StringUtils::beginsWith(s, "word-acquired")){
			int player = s[s.size()-1] - '0';
			std::cout << "word acquired by player " << player << std::endl;
			++m_playersPassed;
			if(m_playersPassed == 1){
				++m_playerScore[player];
				m_scoreWidgets[player]->setText(Radiant::StringUtils::stringify(m_playerScore[player]));
			}
			Radiant::BinaryData data;
			data.writeInt32(player);
			eventSend("clear-widget", data);
		}
		else if(Radiant::StringUtils::beginsWith(s, "clear-widget")){
			int player = bd.readInt32();
			m_collectedWords[player]->addWord((wordReader.getWord(player, m_currentsentence, m_currentword)).word);
			m_currentWordWidgets[player]->raiseFlag(RoundTextBox::DELETE_ME);
			if(m_playersPassed == m_players){
				// Level clear, go to next
				initializeLevel();
			}
		}
		else {
			Parent::processMessage(msg, bd);
		}
	}

	void gotoLevel(int sentenceid, int wordid) {
		Nimble::RandomUniform rnd(55);
		m_playersPassed = 0;
		// delete everything, create again

		float maxH = 10;
		MyApplication& app = *MyApplication::me;

		MultiWidgets::Widget::ChildIterator it;
		for (it = app.root()->childBegin(); it != app.root()->childEnd(); ++it) {
			if (dynamic_cast<RoundTextBox*>(*it))
				app.root()->deleteChild(*it);
		}
		for (it = app.overlay()->childBegin(); it != app.overlay()->childEnd(); ++it) {
			if (dynamic_cast<RoundTextBox*>(*it))
				app.overlay()->deleteChild(*it);
		}

		if (sentenceid > wordReader.maxSentence())
			return;
		
		// Create words

		for (int i=0; i < m_players; ++i) {
			
			TargetWord word = wordReader.getWord(i, sentenceid, wordid);

			RoundTextBox * tb = new RoundTextBox(app.root(), 0, MultiWidgets::TextBox::HCENTER);
			tb->setCSSClass("FloatingWord");

			tb->setStyle(app.style());
			tb->setText(word.word);
			// Word width and distance is in millimeters, have to calculate pixel width and location
			int pixelWidth = Nimble::Math::Round((double)app.size().maximum() / m_physicalWidth * word.width);
			int x = Nimble::Math::Round((double)app.size().maximum() / m_physicalWidth * word.distance);
			if(i == 0){
				// To left side, calculate from start button left edge to word widget right edge
				int edge = m_startButtons[i]->location().x;
				x = edge - x - pixelWidth;
				if(x < 0){
					x = 0;
					std::cout << "WARNING: Requested distance " << word.distance << " mm does not fit on current screen (width " << m_physicalWidth <<" mm)." << std::endl;
				}
			}
			else{
				// To right side, calculate from start button right edge to word widget left edge
				int edge = m_startButtons[i]->location().x + m_startButtons[i]->size().x;
				x = edge + x;
				if((x + pixelWidth) > app.size().maximum()){
					x = app.size().maximum() - pixelWidth;
					std::cout << "WARNING: Requested distance " << word.distance << " mm does not fit on current screen width (" << m_physicalWidth <<" mm)." << std::endl;
				}
			}
			
			tb->setWidth(pixelWidth);
			tb->setHeight(pixelWidth);
			tb->setAlignFlags(MultiWidgets::TextBox::HCENTER | MultiWidgets::TextBox::VCENTER);
			tb->setLocation(Nimble::Vector2(x, app.size().minimum()/2 - pixelWidth/2));
			
			std::cout << "Widget calculated pixel width: " << pixelWidth << " location: x " << tb->location().x << " y " << tb->location().y << std::endl;
			
			tb->raiseFlag(RoundTextBox::LOCK_DEPTH);
			tb->addOperator(new MultiWidgets::StayInsideParentOperator());
			std::string eventname = std::string("word-acquired-") + Radiant::StringUtils::stringify(i);
			tb->eventAddListener("interactionbegin", eventname.c_str(), this);
			while(tb->lineCount() > 1){
				int faceSize = tb->faceSize()-1;
				tb->setFaceSize(faceSize);
				if(faceSize == 6) break;
			}
			
			// Add widget to vector to find it later
			m_currentWordWidgets.push_back(tb);
		}
		std::string w;

		m_levelStartedAt = Radiant::TimeStamp::getTime();
	}

	std::vector<std::vector<Line> > m_lines;
	std::vector<std::vector<std::string> > m_sentences;
	std::vector<std::vector<std::string> > m_wordLists;
	int m_currentsentence;
	int m_currentword;

	std::vector<CollectedWordsWidget*> m_collectedWords;
	std::vector<RoundTextBox*> m_startButtons;
	bool m_playerReadyToStart[2];
	int m_playerScore[2];
	//bool m_playerFinished[2];
	std::vector<RoundTextBox*> m_currentWordWidgets;
	std::vector<MultiWidgets::TextBox*> m_scoreWidgets;
	
	Radiant::TimeStamp m_levelStartedAt;

	Valuable::ValueInt m_duplicateCount;
	int m_players;
	int m_playersPassed;
	double m_physicalWidth;

	DistortWidget * d;

	WordReader wordReader;
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
	// Default is the width of Multitouch Cell Advanced in millimeters
	double displayWidth = 1018;

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
		} else if (r == "--players" && (i+1) < argc) {
			players = atoi(argv[++i]);
		} else if (r == "--levels" && (i+1) < argc) {
			levelCount = atoi(argv[++i]);
		} else if (r == "--displaywidth" && (i+1) < argc) {
			// Should be millimeters
			displayWidth = atof(argv[++i]);
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


	WordGameWidget * wg = new WordGameWidget(players, app.root(), d, displayWidth);
	wg->setDepth(-25);
	wg->raiseFlag(WordGameWidget::LOCK_DEPTH);
	wg->setAutoBringToTop(false);

	std::vector<std::string> levels;
	for (int i=1; i <= 12; ++i) {
		levels.push_back(Radiant::StringUtils::stringify(i));
	}

	bool rotation = true;
	float maxH = 10;
	/*
	for (int i=0; i < levelCount; ++i) {
		wg->addLevel(levels[i]);
	}
	*/
	wg->initializeLevel();
	wg->setStyle(app.style());

	return app.run();
}
