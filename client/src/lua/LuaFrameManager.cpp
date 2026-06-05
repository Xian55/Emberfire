#include "stdafx.h"
#include "lua/LuaFrameManager.h"
#include "lua/LuaUI.h"
#include "lua/LuaEngine.h"
#include "lua/LuaEvents.h"

#include "Sprite.h"
#include "Text.h"
#include "PromptBox.h"
#include "ContentMgr.h"
#include "Application.h"
#include "Game.h"
#include "World.h"
#include "Login.h"
#include "ClientPlayer.h"
#include "..\..\SqlConnector\QueryResult.h"
#include "..\..\Shared\Config.h"

#include <assert.h>

// ---------------------------------------------------------------- LuaFrame

LuaFrame::LuaFrame(RenderObject& owner, const int id) : RenderObjectHolder(&owner, id)
{
	setMouseable(false);   // M2: the frame itself isn't a hit target; its regions draw.
	setMultiInput(true);   // route input to ALL children (buttons + editboxes), not just the topmost one.
}

void LuaFrame::setLogicalSize(const int w, const int h)
{
	m_w = w;
	m_h = h;
	m_bottomRightCorner = { m_topLeftCorner.x + w, m_topLeftCorner.y + h };
}

// ---------------------------------------------------------------- LuaTexture

LuaTexture::LuaTexture(RenderObject& owner, const int id) : RenderObject(&owner, id)
{
	setMouseable(false);
}

void LuaTexture::setTexture(const std::string& textureName)
{
	m_sprite = sContentMgr->spawnSprite(textureName);
}

void LuaTexture::setScaleSize(const int w, const int h)
{
	m_w = w;
	m_h = h;
}

void LuaTexture::setColor(const int r, const int g, const int b, const int a)
{
	if (m_sprite)
		m_sprite->setColor(sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g),
		                             static_cast<sf::Uint8>(b), static_cast<sf::Uint8>(a)));
}

sf::Vector2i LuaTexture::naturalSize() const
{
	if (m_w > 0 && m_h > 0)
		return { m_w, m_h };
	if (!m_sprite)
		return { 0, 0 };
	const auto lb = m_sprite->getLocalBounds();
	return { static_cast<int>(lb.width), static_cast<int>(lb.height) };
}

void LuaTexture::render()
{
	if (!m_sprite)
		return;

	if (m_w > 0 && m_h > 0)
	{
		const auto lb = m_sprite->getLocalBounds();
		if (lb.width > 0.f && lb.height > 0.f)
			m_sprite->setScale(m_w / lb.width, m_h / lb.height);
	}

	m_sprite->render({ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) });
}

// ---------------------------------------------------------------- LuaFontString

LuaFontString::LuaFontString(RenderObject& owner, const int id) : RenderObject(&owner, id)
{
	setMouseable(false);
	m_text = make_unique<Text>(sContentMgr->getFont(FontId::Helvetica));
	m_text->setCharacterSize(16);
	m_text->setFillColor(sf::Color::White);
	m_text->setOutlineColor(sf::Color::Black);
	m_text->setOutlineThickness(1.f);
}

void LuaFontString::setText(const std::string& str)
{
	m_text->setString(str);
}

void LuaFontString::setFontSize(const int n)
{
	m_text->setCharacterSize(n);
}

void LuaFontString::setColor(const int r, const int g, const int b, const int a)
{
	m_text->setFillColor(sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g),
	                               static_cast<sf::Uint8>(b), static_cast<sf::Uint8>(a)));
	// fade the outline with the fill so the text fades cleanly
	m_text->setOutlineColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(a / 2)));
}

static Assets::FontId fontIdFromName(const std::string& n)
{
	if (n == "Ringbearer")    return Assets::FontId::Ringbearer;
	if (n == "Palatino")      return Assets::FontId::Palatino;
	if (n == "PalatinoBold")  return Assets::FontId::PalatinoBold;
	if (n == "Trebuchet")     return Assets::FontId::Trebuchet;
	if (n == "Cambria")       return Assets::FontId::Cambria;
	if (n == "FrizRegular")   return Assets::FontId::FrizRegular;
	if (n == "FrizBold")      return Assets::FontId::FrizBold;
	if (n == "Constantia")    return Assets::FontId::Constantia;
	if (n == "Arial")         return Assets::FontId::Arial;
	if (n == "Fontin")        return Assets::FontId::Fontin;
	return Assets::FontId::Helvetica;
}

void LuaFontString::setFont(const std::string& fontName)
{
	// getFont returns a shared_ptr; the font is cached in ContentMgr so it outlives the Text reference.
	m_text->setFont(*sContentMgr->getFont(fontIdFromName(fontName)));
}

sf::Vector2i LuaFontString::textSize() const
{
	const auto b = m_text->getGlobalBounds();
	return { static_cast<int>(b.width), static_cast<int>(b.height) };
}

void LuaFontString::render()
{
	m_text->draw(m_topLeftCorner.x, m_topLeftCorner.y);
}

// ---------------------------------------------------------------- LuaButton

LuaButton::LuaButton(RenderObject& owner, const int id) : RenderObjectHolder(&owner, id)
{
	// mouseable by default => it is a hit target. Children (bars/text/textures) anchor to it.
	setMultiInput(true);   // all children receive input, not just the topmost
}

void LuaButton::setTexture(const std::string& textureName)
{
	m_sprite = sContentMgr->spawnSprite(textureName);
	if (m_sprite && m_w == 0 && m_h == 0)
	{
		const auto lb = m_sprite->getLocalBounds();
		m_w = static_cast<int>(lb.width);
		m_h = static_cast<int>(lb.height);
	}
}

void LuaButton::setHoverTexture(const std::string& textureName)
{
	m_hoverSprite = sContentMgr->spawnSprite(textureName);
}

bool LuaButton::popClicked()
{
	const bool c = m_clicked;
	m_clicked = false;
	return c;
}

void LuaButton::input()
{
	RenderObjectHolder::input();   // route input to children first

	// Hit-test our own rect directly (MouseableNode), not the holder override: a mouseable child can steal
	// topMostMousedOver and break the holder's isMousedOver, so the click would never register.
	if (MouseableNode::isMousedOver(true) && sApplication->mouseUp(sf::Mouse::Left))
		m_clicked = true;
}

void LuaButton::render()
{
	m_bottomRightCorner = { m_topLeftCorner.x + m_w, m_topLeftCorner.y + m_h };

	const sf::Vector2f pos{ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) };
	const bool hovered = MouseableNode::isMousedOver(true);

	if (hovered && m_hoverSprite)
	{
		m_hoverSprite->render(pos);   // designer-provided hover art (e.g. login_hover.png)
	}
	else if (m_sprite)
	{
		// Subtle hover affordance: full-bright when pointed at, slightly dimmed otherwise.
		m_sprite->setColor(hovered ? sf::Color(255, 255, 255) : sf::Color(225, 225, 225));
		m_sprite->render(pos);
	}

	RenderObjectHolder::render();   // draw children (bars, fontstrings, textures) on top of the bg
}

// ---------------------------------------------------------------- LuaStatusBar

LuaStatusBar::LuaStatusBar(RenderObject& owner, const int id) : RenderObject(&owner, id)
{
	setMouseable(false);
}

void LuaStatusBar::setTexture(const std::string& textureName)
{
	m_fill = sContentMgr->spawnSprite(textureName);
}

void LuaStatusBar::setBarSize(const int w, const int h) { m_w = w; m_h = h; }
void LuaStatusBar::setMinMax(const float mn, const float mx) { m_min = mn; m_max = mx; }
void LuaStatusBar::setValue(const float v) { m_value = v; }

void LuaStatusBar::setColor(const int r, const int g, const int b, const int a)
{
	if (m_fill)
		m_fill->setColor(sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g),
		                           static_cast<sf::Uint8>(b), static_cast<sf::Uint8>(a)));
}

void LuaStatusBar::render()
{
	if (!m_fill || m_w <= 0 || m_h <= 0)
		return;

	const float range = m_max - m_min;
	float pct = (range > 0.f) ? (m_value - m_min) / range : 0.f;
	if (pct < 0.f) pct = 0.f;
	if (pct > 1.f) pct = 1.f;
	if (pct <= 0.f)
		return;

	const auto lb = m_fill->getLocalBounds();
	if (lb.width > 0.f && lb.height > 0.f)
		m_fill->setScale((m_w * pct) / lb.width, m_h / lb.height);

	m_fill->render({ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) });
}

// ---------------------------------------------------------------- LuaEditBox

// The currently focused editbox. Application clears the current-prompt every frame, so the focused box
// must re-assert it each frame (mirrors how the C++ Login/Console/GameChat keep their PromptBox focused).
static LuaEditBox* s_focusedEditBox = nullptr;

LuaEditBox::~LuaEditBox()
{
	if (s_focusedEditBox == this)
		s_focusedEditBox = nullptr;
}

LuaEditBox::LuaEditBox(RenderObject& owner, const int id) : RenderObjectHolder(&owner, id)
{
	setMultiInput(true);
	// Wrap a PromptBox as a child anchored to our top-left. Child id is local to this holder.
	m_prompt = make_shared<PromptBox>(*this, 1, Assets::FontId::Palatino, nullptr,
	                                  sf::Vector2i{}, 400, sf::Color(220, 194, 171, 255), false);
	m_prompt->setPromptCharacterSize(20);
	m_prompt->setAnchor(&getTopLeftCornerRef());
	m_prompt->setOffset({ 0, 0 });
	addRenderObject(m_prompt);
}

void LuaEditBox::setText(const std::string& str)       { m_prompt->setContent(str); }
std::string LuaEditBox::getText() const                { return m_prompt->getContent(); }
void LuaEditBox::setMaxLetters(const int n)            { m_prompt->setPromptMaxStrLen(n); }
void LuaEditBox::setPassword(const bool masked)        { m_prompt->setSafetyRender(masked); }
void LuaEditBox::setNumeric(const bool v)              { m_prompt->setNumbersOnly(v); }
void LuaEditBox::setFontSize(const int n)              { m_prompt->setPromptCharacterSize(n); }

void LuaEditBox::setTextColor(const int r, const int g, const int b, const int a)
{
	m_textColor = sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g),
	                        static_cast<sf::Uint8>(b), static_cast<sf::Uint8>(a));
	m_prompt->setColor(m_textColor);
}

void LuaEditBox::setBoxSize(const int w, const int h)
{
	m_w = w;
	m_h = h;
	m_prompt->setMaxWidth(w);
}

bool LuaEditBox::popSubmitted()
{
	const bool s = m_submitted;
	m_submitted = false;
	return s;
}

void LuaEditBox::input()
{
	// Take focus on a left release over our own rect (hit-test MouseableNode directly — a mouseable child
	// can otherwise steal topMostMousedOver and break the holder's isMousedOver).
	if (MouseableNode::isMousedOver(true) && sApplication->mouseUp(sf::Mouse::Left))
		s_focusedEditBox = this;

	// Application zeroes the current-prompt at the start of every input(); re-assert it here EACH frame
	// while focused, and BEFORE routing to the PromptBox child below (which reads isCurrentPrompt()).
	if (s_focusedEditBox == this)
		sApplication->setCurrentPrompt(static_cast<const void*>(m_prompt.get()));

	RenderObjectHolder::input();   // the PromptBox child now reads typed text (it is the current prompt)

	// Enter submits (PromptBox has no enter button, so detect Return while focused).
	if (m_prompt->isCurrentPrompt() && sApplication->keyUp(sf::Keyboard::Return))
		m_submitted = true;
}

void LuaEditBox::render()
{
	m_bottomRightCorner = { m_topLeftCorner.x + m_w, m_topLeftCorner.y + m_h };

	// Subtle hover affordance: brighten the field text slightly when pointed at.
	if (MouseableNode::isMousedOver(true))
	{
		const auto up = [](sf::Uint8 c) -> sf::Uint8 { const int v = c + 35; return v > 255 ? 255 : static_cast<sf::Uint8>(v); };
		m_prompt->setColor(sf::Color(up(m_textColor.r), up(m_textColor.g), up(m_textColor.b), m_textColor.a));
	}
	else
	{
		m_prompt->setColor(m_textColor);
	}

	RenderObjectHolder::render();   // draws the PromptBox child
}

// ---------------------------------------------------------------- LuaFrameManager

LuaFrameManager* LuaFrameManager::s_instance = nullptr;

void LuaFrameManager::debugDumpBounds()
{
	if (s_instance)
		s_instance->dumpBounds();
}

// Log every VISIBLE Lua widget's type + position + size. Used for layout debugging. A LOG dump (not an
// on-screen overlay) because drawing raw sfml shapes mid-frame corrupts the game's GL render state.
void LuaFrameManager::dumpBounds() const
{
	blog(Logger::LOG_INFO, "[lua] --- widget bounds (visible) ---");
	for (auto& [h, obj] : m_objects)
	{
		bool visible = true;
		for (RenderObject* o = obj.get(); o != nullptr; o = o->getOwner())
		{
			if (o->isHidden() || o->isForceHidden()) { visible = false; break; }
			if (o->getOwner() == o) break;
		}
		if (!visible)
			continue;

		const auto& tl = obj->getTopLeftCornerRef();
		const auto& br = obj->getBottomRightCornerRef();
		const char* ty = dynamic_cast<LuaEditBox*>(obj.get())    ? "EditBox"
		               : dynamic_cast<LuaButton*>(obj.get())     ? "Button"
		               : dynamic_cast<LuaStatusBar*>(obj.get())  ? "StatusBar"
		               : dynamic_cast<LuaTexture*>(obj.get())    ? "Texture"
		               : dynamic_cast<LuaFontString*>(obj.get()) ? "FontString"
		               : dynamic_cast<LuaFrame*>(obj.get())      ? "Frame" : "?";
		blog(Logger::LOG_INFO, "[lua]  h=%d %-10s pos=(%d,%d) size=(%d,%d)",
			h, ty, tl.x, tl.y, br.x - tl.x, br.y - tl.y);
	}
}

LuaFrameManager::LuaFrameManager(RenderObject& owner, const int id) : RenderObjectHolder(&owner, id)
{
	setMultiInput(true);
	setMouseable(false);
	s_instance = this;
	sLua->clearFrames();   // fresh World => drop any handlers from a previous one
}

LuaFrameManager::~LuaFrameManager()
{
	if (s_instance == this)
		s_instance = nullptr;
	sLua->clearFrames();   // frames are gone; drop their handlers so nothing stale fires
}

RenderObject* LuaFrameManager::lookup(int handle) const
{
	auto it = m_objects.find(handle);
	return (it == m_objects.end()) ? nullptr : it->second.get();
}

void LuaFrameManager::attachChild(RenderObjectHolder& parent, shared_ptr<RenderObject> child, int handle)
{
	// Equivalent to the protected attachObj(), but callable across holder instances (all public API).
	child->setAnchor(&parent.getTopLeftCornerRef());
	child->setOffset({ 0, 0 });
	parent.addRenderObject(child);
	m_objects[handle] = child;
}

int LuaFrameManager::createFrame(int parentHandle)
{
	RenderObjectHolder* parent = this;
	if (parentHandle != 0)
	{
		parent = dynamic_cast<RenderObjectHolder*>(lookup(parentHandle));
		if (!parent)
			return 0;
	}

	const int h = m_nextHandle++;
	attachChild(*parent, make_shared<LuaFrame>(*parent, h), h);
	return h;
}

int LuaFrameManager::createTexture(int frameHandle)
{
	auto* frame = dynamic_cast<RenderObjectHolder*>(lookup(frameHandle));
	if (!frame)
		return 0;

	const int h = m_nextHandle++;
	attachChild(*frame, make_shared<LuaTexture>(*frame, h), h);
	return h;
}

int LuaFrameManager::createFontString(int frameHandle)
{
	auto* frame = dynamic_cast<RenderObjectHolder*>(lookup(frameHandle));
	if (!frame)
		return 0;

	const int h = m_nextHandle++;
	attachChild(*frame, make_shared<LuaFontString>(*frame, h), h);
	return h;
}

int LuaFrameManager::createButton(int parentHandle)
{
	RenderObjectHolder* parent = this;
	if (parentHandle != 0)
	{
		parent = dynamic_cast<RenderObjectHolder*>(lookup(parentHandle));
		if (!parent)
			return 0;
	}

	const int h = m_nextHandle++;
	attachChild(*parent, make_shared<LuaButton>(*parent, h), h);
	return h;
}

int LuaFrameManager::createStatusBar(int parentHandle)
{
	RenderObjectHolder* parent = this;
	if (parentHandle != 0)
	{
		parent = dynamic_cast<RenderObjectHolder*>(lookup(parentHandle));
		if (!parent)
			return 0;
	}

	const int h = m_nextHandle++;
	attachChild(*parent, make_shared<LuaStatusBar>(*parent, h), h);
	return h;
}

int LuaFrameManager::createEditBox(int parentHandle)
{
	RenderObjectHolder* parent = this;
	if (parentHandle != 0)
	{
		parent = dynamic_cast<RenderObjectHolder*>(lookup(parentHandle));
		if (!parent)
			return 0;
	}

	const int h = m_nextHandle++;
	attachChild(*parent, make_shared<LuaEditBox>(*parent, h), h);
	return h;
}

int LuaFrameManager::popClickedHandle()
{
	for (auto& [h, obj] : m_objects)
		if (auto b = dynamic_cast<LuaButton*>(obj.get()))
			if (b->popClicked())
				return h;
	return 0;
}

int LuaFrameManager::popSubmittedHandle()
{
	for (auto& [h, obj] : m_objects)
		if (auto e = dynamic_cast<LuaEditBox*>(obj.get()))
			if (e->popSubmitted())
				return h;
	return 0;
}

std::string LuaFrameManager::getText(int handle) const
{
	if (auto e = dynamic_cast<LuaEditBox*>(lookup(handle)))
		return e->getText();
	return std::string();
}

void LuaFrameManager::setEditBoxPassword(int handle, bool masked)
{
	if (auto e = dynamic_cast<LuaEditBox*>(lookup(handle))) e->setPassword(masked);
}

void LuaFrameManager::setEditBoxMaxLen(int handle, int n)
{
	if (auto e = dynamic_cast<LuaEditBox*>(lookup(handle))) e->setMaxLetters(n);
}

void LuaFrameManager::setEditBoxNumeric(int handle, bool v)
{
	if (auto e = dynamic_cast<LuaEditBox*>(lookup(handle))) e->setNumeric(v);
}

void LuaFrameManager::setEditBoxFontSize(int handle, int n)
{
	auto* o = lookup(handle);
	if (auto e = dynamic_cast<LuaEditBox*>(o))         e->setFontSize(n);
	else if (auto fs = dynamic_cast<LuaFontString*>(o)) fs->setFontSize(n);
}

void LuaFrameManager::setEditBoxColor(int handle, int r, int g, int b, int a)
{
	auto* o = lookup(handle);
	if (auto e = dynamic_cast<LuaEditBox*>(o))         e->setTextColor(r, g, b, a);
	else if (auto fs = dynamic_cast<LuaFontString*>(o)) fs->setColor(r, g, b, a);   // FontString:SetTextColor
}

void LuaFrameManager::setVertexColor(int handle, int r, int g, int b, int a)
{
	if (auto t = dynamic_cast<LuaTexture*>(lookup(handle))) t->setColor(r, g, b, a);
}

void LuaFrameManager::setFont(int handle, const std::string& fontName)
{
	if (auto fs = dynamic_cast<LuaFontString*>(lookup(handle))) fs->setFont(fontName);
}

void LuaFrameManager::setMinMax(int handle, float mn, float mx)
{
	if (auto s = dynamic_cast<LuaStatusBar*>(lookup(handle))) s->setMinMax(mn, mx);
}

void LuaFrameManager::setValue(int handle, float v)
{
	if (auto s = dynamic_cast<LuaStatusBar*>(lookup(handle))) s->setValue(v);
}

void LuaFrameManager::setColor(int handle, int r, int g, int b, int a)
{
	if (auto s = dynamic_cast<LuaStatusBar*>(lookup(handle))) s->setColor(r, g, b, a);
}

sf::Vector2i LuaFrameManager::sizeOf(RenderObject* o) const
{
	if (auto f = dynamic_cast<LuaFrame*>(o))      return f->logicalSize();
	if (auto t = dynamic_cast<LuaTexture*>(o))    return t->naturalSize();
	if (auto s = dynamic_cast<LuaFontString*>(o)) return s->textSize();
	if (auto b = dynamic_cast<LuaButton*>(o))     return b->hitSize();
	if (auto sb = dynamic_cast<LuaStatusBar*>(o)) return sb->barSize();
	if (auto e = dynamic_cast<LuaEditBox*>(o))    return e->boxSize();
	return { 0, 0 };
}

void LuaFrameManager::setPoint(int handle, int point, int relHandle, int relPoint, float x, float y)
{
	auto* obj = lookup(handle);
	if (!obj)
		return;

	RenderObject* rel = (relHandle != 0) ? lookup(relHandle) : obj->getOwner();

	const sf::Vector2i parentSize = (rel == this || rel == nullptr)
		? sf::Vector2i{ sApplication->sW(), sApplication->sH() }   // top-level => relative to the screen
		: sizeOf(rel);
	const sf::Vector2i childSize = sizeOf(obj);

	sf::Vector2i off{ static_cast<int>(x), static_cast<int>(y) };
	if (point == LuaUI::Center)
	{
		off.x += parentSize.x / 2 - childSize.x / 2;
		off.y += parentSize.y / 2 - childSize.y / 2;
	}

	if (relHandle != 0 && rel)
		obj->setAnchor(&rel->getTopLeftCornerRef());

	obj->setOffset(off);
}

void LuaFrameManager::setSize(int handle, float w, float h)
{
	auto* o = lookup(handle);
	if (auto f = dynamic_cast<LuaFrame*>(o))          f->setLogicalSize(static_cast<int>(w), static_cast<int>(h));
	else if (auto t = dynamic_cast<LuaTexture*>(o))   t->setScaleSize(static_cast<int>(w), static_cast<int>(h));
	else if (auto b = dynamic_cast<LuaButton*>(o))    b->setHitSize(static_cast<int>(w), static_cast<int>(h));
	else if (auto sb = dynamic_cast<LuaStatusBar*>(o)) sb->setBarSize(static_cast<int>(w), static_cast<int>(h));
	else if (auto e = dynamic_cast<LuaEditBox*>(o))   e->setBoxSize(static_cast<int>(w), static_cast<int>(h));
}

void LuaFrameManager::setText(int handle, const std::string& text)
{
	auto* o = lookup(handle);
	if (auto fs = dynamic_cast<LuaFontString*>(o))    fs->setText(text);
	else if (auto e = dynamic_cast<LuaEditBox*>(o))   e->setText(text);
}

void LuaFrameManager::setTexture(int handle, const std::string& textureName)
{
	auto* o = lookup(handle);
	if (auto t = dynamic_cast<LuaTexture*>(o))         t->setTexture(textureName);
	else if (auto b = dynamic_cast<LuaButton*>(o))     b->setTexture(textureName);
	else if (auto sb = dynamic_cast<LuaStatusBar*>(o)) sb->setTexture(textureName);
}

void LuaFrameManager::setHoverTexture(int handle, const std::string& textureName)
{
	if (auto b = dynamic_cast<LuaButton*>(lookup(handle))) b->setHoverTexture(textureName);
}

void LuaFrameManager::show(int handle, bool shown)
{
	if (auto o = lookup(handle))
		o->setHidden(!shown);
}

void LuaFrameManager::clearAll()
{
	clearNow();              // remove + destroy all child render objects (RenderObjectHolder)
	m_objects.clear();
	m_nextHandle = 100000;
}

// ---------------------------------------------------------------- LuaUI:: (sol-free backend)

namespace LuaUI
{
	int createFrame(int parentHandle)
	{
		auto* m = LuaFrameManager::instance();
		return m ? m->createFrame(parentHandle) : 0;
	}

	int createTexture(int frameHandle)
	{
		auto* m = LuaFrameManager::instance();
		return m ? m->createTexture(frameHandle) : 0;
	}

	int createFontString(int frameHandle)
	{
		auto* m = LuaFrameManager::instance();
		return m ? m->createFontString(frameHandle) : 0;
	}

	int createButton(int parentHandle)
	{
		auto* m = LuaFrameManager::instance();
		return m ? m->createButton(parentHandle) : 0;
	}

	int popClickedHandle()
	{
		auto* m = LuaFrameManager::instance();
		return m ? m->popClickedHandle() : 0;
	}

	void clearAllFrames()
	{
		if (auto* m = LuaFrameManager::instance())
			m->clearAll();
	}

	int createStatusBar(int parentHandle)
	{
		auto* m = LuaFrameManager::instance();
		return m ? m->createStatusBar(parentHandle) : 0;
	}

	int createEditBox(int parentHandle)
	{
		auto* m = LuaFrameManager::instance();
		return m ? m->createEditBox(parentHandle) : 0;
	}

	int popSubmittedHandle()
	{
		auto* m = LuaFrameManager::instance();
		return m ? m->popSubmittedHandle() : 0;
	}

	std::string getText(int handle)
	{
		auto* m = LuaFrameManager::instance();
		return m ? m->getText(handle) : std::string();
	}

	void setEditBoxPassword(int handle, bool masked) { if (auto* m = LuaFrameManager::instance()) m->setEditBoxPassword(handle, masked); }
	void setEditBoxMaxLen(int handle, int n)         { if (auto* m = LuaFrameManager::instance()) m->setEditBoxMaxLen(handle, n); }
	void setEditBoxNumeric(int handle, bool v)       { if (auto* m = LuaFrameManager::instance()) m->setEditBoxNumeric(handle, v); }
	void setEditBoxFontSize(int handle, int n)       { if (auto* m = LuaFrameManager::instance()) m->setEditBoxFontSize(handle, n); }
	void setEditBoxColor(int handle, int r, int g, int b, int a) { if (auto* m = LuaFrameManager::instance()) m->setEditBoxColor(handle, r, g, b, a); }
	void setVertexColor(int handle, int r, int g, int b, int a)  { if (auto* m = LuaFrameManager::instance()) m->setVertexColor(handle, r, g, b, a); }
	void setFont(int handle, const std::string& fontName)        { if (auto* m = LuaFrameManager::instance()) m->setFont(handle, fontName); }

	void setMinMax(int handle, float mn, float mx) { if (auto* m = LuaFrameManager::instance()) m->setMinMax(handle, mn, mx); }
	void setValue(int handle, float v)             { if (auto* m = LuaFrameManager::instance()) m->setValue(handle, v); }
	void setBarColor(int handle, int r, int g, int b, int a) { if (auto* m = LuaFrameManager::instance()) m->setColor(handle, r, g, b, a); }

	void setPoint(int handle, int point, int relHandle, int relPoint, float xOfs, float yOfs)
	{
		if (auto* m = LuaFrameManager::instance())
			m->setPoint(handle, point, relHandle, relPoint, xOfs, yOfs);
	}

	void setSize(int handle, float w, float h)
	{
		if (auto* m = LuaFrameManager::instance())
			m->setSize(handle, w, h);
	}

	void setText(int handle, const std::string& text)
	{
		if (auto* m = LuaFrameManager::instance())
			m->setText(handle, text);
	}

	void setTexture(int handle, const std::string& textureName)
	{
		if (auto* m = LuaFrameManager::instance())
			m->setTexture(handle, textureName);
	}

	void setHoverTexture(int handle, const std::string& textureName)
	{
		if (auto* m = LuaFrameManager::instance())
			m->setHoverTexture(handle, textureName);
	}

	void show(int handle, bool shown)
	{
		if (auto* m = LuaFrameManager::instance())
			m->show(handle, shown);
	}

	bool valid(int handle)
	{
		auto* m = LuaFrameManager::instance();
		return m && m->valid(handle);
	}

	// ---- game-state getters ----

	static Game* currentGame()
	{
		return dynamic_pointer_cast<Game>(sApplication->getRenderObject(Application::RoGame)).get();
	}

	static World* currentWorld()
	{
		auto* game = currentGame();
		if (!game)
			return nullptr;
		return dynamic_pointer_cast<World>(game->getRenderObject(Game::RoWorld)).get();
	}

	static ClientUnit* resolveUnit(const std::string& token)
	{
		auto* w = currentWorld();
		if (!w)
			return nullptr;
		if (token == "player")
			return w->myself();
		if (token == "target")
			return w->selectedUnit();
		return nullptr;
	}

	int unitHealth(const std::string& token)    { auto* u = resolveUnit(token); return u ? u->getHealth() : 0; }
	int unitHealthMax(const std::string& token) { auto* u = resolveUnit(token); return u ? u->getMaxHealth() : 0; }
	int unitLevel(const std::string& token)     { auto* u = resolveUnit(token); return u ? u->getLevel() : 0; }
	int unitPower(const std::string& token)     { auto* u = resolveUnit(token); return u ? u->getMana() : 0; }
	int unitPowerMax(const std::string& token)  { auto* u = resolveUnit(token); return u ? u->getMaxMana() : 0; }
	std::string unitName(const std::string& token) { auto* u = resolveUnit(token); return u ? u->getName() : std::string(); }
	bool unitExists(const std::string& token)   { return resolveUnit(token) != nullptr; }

	int playerXP()
	{
		auto* w = currentWorld();
		return (w && w->myself()) ? w->myself()->getVariable(ObjDefines::Variable::Progression) : 0;
	}

	int playerMaxXP()
	{
		auto* w = currentWorld();
		if (!w || !w->myself())
			return 0;
		const int req = atoi(sContentMgr->db("player_exp_levels").data(w->myself()->getLevel() + 1, "exp").c_str());
		return req > 0 ? req : 0;
	}

	void targetUnit(const std::string& token)
	{
		auto* w = currentWorld();
		if (!w) return;
		auto* u = resolveUnit(token);
		w->setSelectedGuid(u ? u->getGuid() : 0);
	}

	void clearTarget() { if (auto* w = currentWorld()) w->setSelectedGuid(0); }

	void setGameFrameShown(const std::string& name, bool shown)
	{
		// Game-stage screens (login / character) resolve against Game; HUD frames against World.
		if (auto* g = currentGame())
		{
			const int gid = (name == "LoginScreen")      ? Game::RoLogin
			              : (name == "CharSelectScreen")  ? Game::RoCharacterSelection
			              : (name == "CharCreateScreen")  ? Game::RoCharacterCreation : 0;
			if (gid)
			{
				if (auto ro = g->getRenderObject(gid))
					ro->setForceHidden(!shown);
				return;
			}
		}

		auto* w = currentWorld();
		if (!w) return;
		const int id = (name == "PlayerFrame") ? World::PlayerUnitFrame
		             : (name == "TargetFrame") ? World::TargetUnitFrame
		             : (name == "XPBar")       ? World::ToolbarXpObj : 0;
		if (!id) return;
		if (auto ro = w->getRenderObject(id))
			ro->setForceHidden(!shown);   // survives the window re-showing itself each frame
	}

	// ---- login command/getter (drives the C++ Login while it is force-hidden) ----

	void submitLogin(const std::string& user, const std::string& pass, bool remember)
	{
		auto* g = currentGame();
		if (!g) return;
		auto login = dynamic_pointer_cast<Login>(g->getRenderObject(Game::RoLogin));
		if (!login) return;
		std::string err;
		if (!login->performLogin(user, pass, remember, err))
			sLua->fire(LuaEvents::LOGIN_RESULT, err);
		// success => the server's Validate reply drives the stage change (CHARSELECT_SHOWN).
	}

	std::string getSavedLogin()
	{
		return sConfig->getString("UI", "DefaultLogin", "");
	}

	int screenWidth()  { return sApplication->sW(); }
	int screenHeight() { return sApplication->sH(); }

	void setDebugBounds(bool v) { if (v) LuaFrameManager::debugDumpBounds(); }

	// ---- z-order (WoW SetFrameLevel / Raise / Lower) ----

	static RenderObjectHolder* parentHolderOf(int handle)
	{
		auto* m = LuaFrameManager::instance();
		auto* o = m ? m->lookup(handle) : nullptr;
		return o ? dynamic_cast<RenderObjectHolder*>(o->getOwner()) : nullptr;
	}

	void setFrameLevel(int handle, int level)
	{
		auto* m = LuaFrameManager::instance();
		if (auto* o = m ? m->lookup(handle) : nullptr)
		{
			o->setZLevel(level);
			if (auto* p = dynamic_cast<RenderObjectHolder*>(o->getOwner()))
				p->sortByZLevel();
		}
	}

	int getFrameLevel(int handle)
	{
		auto* m = LuaFrameManager::instance();
		auto* o = m ? m->lookup(handle) : nullptr;
		return o ? o->getZLevel() : 0;
	}

	void raiseFrame(int handle) { if (auto* p = parentHolderOf(handle)) p->raiseChild(handle); }
	void lowerFrame(int handle) { if (auto* p = parentHolderOf(handle)) p->lowerChild(handle); }

	// Re-sort the root (top-level) frames by their z-level. Called after all UI + addons load so that
	// level-tagged frames (e.g. the loading screen) sit above default-level frames added later (addons).
	void sortRootFrames() { if (auto* m = LuaFrameManager::instance()) m->sortByZLevel(); }
}
