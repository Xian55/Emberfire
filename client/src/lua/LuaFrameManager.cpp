#include "stdafx.h"
#include "lua/LuaFrameManager.h"
#include "lua/LuaUI.h"
#include "lua/LuaEngine.h"

#include "Sprite.h"
#include "Text.h"
#include "ContentMgr.h"
#include "Application.h"
#include "Game.h"
#include "World.h"
#include "ClientPlayer.h"

#include <assert.h>

// ---------------------------------------------------------------- LuaFrame

LuaFrame::LuaFrame(RenderObject& owner, const int id) : RenderObjectHolder(&owner, id)
{
	setMouseable(false);   // M2: the frame itself isn't a hit target; its regions draw.
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

LuaButton::LuaButton(RenderObject& owner, const int id) : RenderObject(&owner, id)
{
	// mouseable by default => it is a hit target.
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

bool LuaButton::popClicked()
{
	const bool c = m_clicked;
	m_clicked = false;
	return c;
}

void LuaButton::input()
{
	// A left release while this is the top-most moused-over object counts as a click.
	if (isMousedOver(true) && sApplication->mouseUp(sf::Mouse::Left))
		m_clicked = true;
}

void LuaButton::render()
{
	m_bottomRightCorner = { m_topLeftCorner.x + m_w, m_topLeftCorner.y + m_h };
	if (m_sprite)
		m_sprite->render({ static_cast<float>(m_topLeftCorner.x), static_cast<float>(m_topLeftCorner.y) });
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

// ---------------------------------------------------------------- LuaFrameManager

LuaFrameManager* LuaFrameManager::s_instance = nullptr;

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

int LuaFrameManager::popClickedHandle()
{
	for (auto& [h, obj] : m_objects)
		if (auto b = dynamic_cast<LuaButton*>(obj.get()))
			if (b->popClicked())
				return h;
	return 0;
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
}

void LuaFrameManager::setText(int handle, const std::string& text)
{
	if (auto fs = dynamic_cast<LuaFontString*>(lookup(handle)))
		fs->setText(text);
}

void LuaFrameManager::setTexture(int handle, const std::string& textureName)
{
	auto* o = lookup(handle);
	if (auto t = dynamic_cast<LuaTexture*>(o))         t->setTexture(textureName);
	else if (auto b = dynamic_cast<LuaButton*>(o))     b->setTexture(textureName);
	else if (auto sb = dynamic_cast<LuaStatusBar*>(o)) sb->setTexture(textureName);
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

	static World* currentWorld()
	{
		auto game = dynamic_pointer_cast<Game>(sApplication->getRenderObject(Application::RoGame));
		if (!game)
			return nullptr;
		return dynamic_pointer_cast<World>(game->getRenderObject(Game::RoWorld)).get();
	}

	static ClientPlayer* resolveUnit(const std::string& token)
	{
		auto* w = currentWorld();
		if (!w)
			return nullptr;
		if (token == "player")
			return w->myself();
		return nullptr;   // "target"/party tokens: later
	}

	int unitHealth(const std::string& token)    { auto* u = resolveUnit(token); return u ? u->getHealth() : 0; }
	int unitHealthMax(const std::string& token) { auto* u = resolveUnit(token); return u ? u->getMaxHealth() : 0; }
	int unitLevel(const std::string& token)     { auto* u = resolveUnit(token); return u ? u->getLevel() : 0; }
}
