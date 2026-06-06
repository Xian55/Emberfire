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
#include "ClientUnit.h"
#include "UnitFrame.h"
#include "ContextMenu.h"
#include "Tooltip.h"
#include "..\..\SqlConnector\QueryResult.h"
#include "..\..\Shared\Config.h"

#include <assert.h>
#include <climits>
#include <algorithm>

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
	m_color = sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g),
	                    static_cast<sf::Uint8>(b), static_cast<sf::Uint8>(a));
	if (m_sprite)
		m_sprite->setColor(m_color);
}

void LuaTexture::setAlpha(const int a)
{
	m_color.a = static_cast<sf::Uint8>(a);
	if (m_sprite)
		m_sprite->setColor(m_color);
}

void LuaTexture::setTexCoord(const float l, const float r, const float t, const float b)
{
	if (!m_sprite || !m_sprite->getTexture())
		return;
	const auto sz = m_sprite->getTexture()->getSize();
	m_sprite->setTextureRect(sf::IntRect(
		static_cast<int>(l * sz.x), static_cast<int>(t * sz.y),
		static_cast<int>((r - l) * sz.x), static_cast<int>((b - t) * sz.y)));
}

void LuaTexture::setCircle(const int radius)
{
	m_circle = radius > 0;
	m_circleRadius = radius;
	if (m_circle && m_sprite && m_sprite->getTexture())
		const_cast<sf::Texture*>(m_sprite->getTexture())->setSmooth(true);   // clean circle edges (mirrors spawnPortrait)
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

	if (m_circle)
	{
		// clipped to a circle centered in the texture's box (box is sized 2r, top-left = center - r)
		const sf::Vector2f center{ static_cast<float>(m_topLeftCorner.x + m_circleRadius),
		                           static_cast<float>(m_topLeftCorner.y + m_circleRadius) };
		m_sprite->renderAsCircle(center, m_circleRadius, { 0, sContentMgr->getPortraitOffset(*m_sprite) });
		return;
	}

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
	m_color = sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g),
	                    static_cast<sf::Uint8>(b), static_cast<sf::Uint8>(a));
	m_text->setFillColor(m_color);
	// fade the outline with the fill so the text fades cleanly
	m_text->setOutlineColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(a / 2)));
}

void LuaFontString::setAlpha(const int a)
{
	m_color.a = static_cast<sf::Uint8>(a);
	m_text->setFillColor(m_color);
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
	m_color = sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g),
	                    static_cast<sf::Uint8>(b), static_cast<sf::Uint8>(a));
	if (m_fill)
		m_fill->setColor(m_color);
}

void LuaStatusBar::setAlpha(const int a)
{
	m_color.a = static_cast<sf::Uint8>(a);
	if (m_fill)
		m_fill->setColor(m_color);
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

void LuaFrameManager::unregisterCtxMenu(const int id, const int childId, const std::string& result)
{
	// We host the Lua-opened unit context menu (so it draws above the Lua HUD). Its reportTo is the live C++
	// UnitFrame in World — not our child — so route there manually, then destroy the menu.
	if (auto menu = dynamic_pointer_cast<ContextMenu>(getRenderObject(m_ctxMenuId)))
	{
		if (auto* g = dynamic_cast<Game*>(getOwner()))
			if (auto w = dynamic_pointer_cast<World>(g->getRenderObject(Game::RoWorld)))
				if (auto rt = w->getRenderObject(menu->getReportToId()))
					rt->notifyCtxMenuClicked(id, result);   // C++ social actions
		m_menuResults.push_back(result);                    // also surface to Lua (Lock/Unlock etc.)
		destroyObjectById(m_ctxMenuId);
		m_ctxMenuId = -1;
	}
}

bool LuaFrameManager::popMenuResult(std::string& out)
{
	if (m_menuResults.empty())
		return false;
	out = m_menuResults.front();
	m_menuResults.erase(m_menuResults.begin());
	return true;
}

bool LuaFrameManager::hitTest(int handle, sf::Vector2i p) const
{
	auto* obj = lookup(handle);
	if (!obj)
		return false;
	// Hidden frames (or any hidden ancestor) are not hoverable.
	for (RenderObject* o = obj; o != nullptr; o = o->getOwner())
	{
		if (o->isHidden() || o->isForceHidden())
			return false;
		if (o->getOwner() == o)
			break;
	}
	const auto& tl = obj->getTopLeftCornerRef();
	const sf::Vector2i sz = sizeOf(obj);   // bottom-right corner ref is unreliable; derive from size
	return p.x >= tl.x && p.x < tl.x + sz.x && p.y >= tl.y && p.y < tl.y + sz.y;
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
	m_parent[h] = parentHandle;
	return h;
}

int LuaFrameManager::createTexture(int frameHandle)
{
	auto* frame = dynamic_cast<RenderObjectHolder*>(lookup(frameHandle));
	if (!frame)
		return 0;

	const int h = m_nextHandle++;
	attachChild(*frame, make_shared<LuaTexture>(*frame, h), h);
	m_parent[h] = frameHandle;
	return h;
}

int LuaFrameManager::createFontString(int frameHandle)
{
	auto* frame = dynamic_cast<RenderObjectHolder*>(lookup(frameHandle));
	if (!frame)
		return 0;

	const int h = m_nextHandle++;
	attachChild(*frame, make_shared<LuaFontString>(*frame, h), h);
	m_parent[h] = frameHandle;
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
	m_parent[h] = parentHandle;
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
	m_parent[h] = parentHandle;
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
	m_parent[h] = parentHandle;
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

// Where an anchor point lands within a box of size sz: TopLeft={0,0}, Center={w/2,h/2}, BottomRight={w,h}.
static sf::Vector2i anchorWithin(int p, const sf::Vector2i& sz)
{
	int ax = 0, ay = 0;
	if (p == LuaUI::Center || p == LuaUI::Top || p == LuaUI::Bottom)              ax = sz.x / 2;
	else if (p == LuaUI::Right || p == LuaUI::TopRight || p == LuaUI::BottomRight) ax = sz.x;
	if (p == LuaUI::Center || p == LuaUI::Left || p == LuaUI::Right)              ay = sz.y / 2;
	else if (p == LuaUI::Bottom || p == LuaUI::BottomLeft || p == LuaUI::BottomRight) ay = sz.y;
	return { ax, ay };
}

void LuaFrameManager::setPoint(int handle, int point, int relHandle, int relPoint, float x, float y)
{
	if (!lookup(handle))
		return;

	auto& list = m_anchors[handle];
	const Anchor a{ point, relHandle, relPoint, static_cast<int>(x), static_cast<int>(y) };
	bool replaced = false;
	for (auto& e : list)
		if (e.point == point) { e = a; replaced = true; break; }   // same point replaces
	if (!replaced)
		list.push_back(a);

	// Recompute every frame only if it depends on another frame, or stretches between two points.
	if (relHandle != 0 || list.size() >= 2)
		m_dynamicAnchored.insert(handle);

	relayout(handle);   // apply immediately (no first-frame flicker)
}

void LuaFrameManager::setAllPoints(int handle, int relHandle)
{
	if (!lookup(handle))
		return;
	auto& list = m_anchors[handle];
	list.clear();
	list.push_back({ LuaUI::TopLeft,     relHandle, LuaUI::TopLeft,     0, 0 });
	list.push_back({ LuaUI::BottomRight, relHandle, LuaUI::BottomRight, 0, 0 });
	m_dynamicAnchored.insert(handle);
	relayout(handle);
}

void LuaFrameManager::clearAllPoints(int handle)
{
	m_anchors.erase(handle);
	m_dynamicAnchored.erase(handle);
}

// Solve the frame's screen position (and size, if two opposing points pin it) from its anchors, then apply
// via setOffset (+ setSize). Reproduces the old single-point math exactly; adds frame-relative + two-point.
void LuaFrameManager::relayout(int handle)
{
	auto* obj = lookup(handle);
	if (!obj)
		return;
	auto it = m_anchors.find(handle);
	if (it == m_anchors.end() || it->second.empty())
		return;

	RenderObject* owner = obj->getOwner();
	const sf::Vector2i ownerTL = (owner && owner != this) ? owner->getTopLeftCornerRef() : sf::Vector2i{ 0, 0 };
	const sf::Vector2i curSize = sizeOf(obj);

	bool haveL = false, haveR = false, haveCX = false, haveT = false, haveB = false, haveCY = false;
	int  L = 0, R = 0, CX = 0, T = 0, B = 0, CY = 0;

	for (const auto& a : it->second)
	{
		RenderObject* rel = (a.relHandle != 0) ? lookup(a.relHandle) : owner;
		const bool relIsScreen = (rel == this || rel == nullptr);
		const sf::Vector2i relTL  = relIsScreen ? sf::Vector2i{ 0, 0 } : rel->getTopLeftCornerRef();
		const sf::Vector2i relSz  = relIsScreen ? sf::Vector2i{ sApplication->sW(), sApplication->sH() } : sizeOf(rel);
		const sf::Vector2i ra     = anchorWithin(a.relPoint, relSz);
		const int tx = relTL.x + ra.x + a.x;   // screen X where the child's `point` should sit
		const int ty = relTL.y + ra.y + a.y;

		const int p = a.point;
		if (p == LuaUI::TopLeft || p == LuaUI::Left || p == LuaUI::BottomLeft)            { haveL = true;  L = tx; }
		else if (p == LuaUI::TopRight || p == LuaUI::Right || p == LuaUI::BottomRight)     { haveR = true;  R = tx; }
		else                                                                              { haveCX = true; CX = tx; }
		if (p == LuaUI::TopLeft || p == LuaUI::Top || p == LuaUI::TopRight)                { haveT = true;  T = ty; }
		else if (p == LuaUI::BottomLeft || p == LuaUI::Bottom || p == LuaUI::BottomRight)  { haveB = true;  B = ty; }
		else                                                                              { haveCY = true; CY = ty; }
	}

	int w = (haveL && haveR) ? (R - L) : curSize.x;
	int h = (haveT && haveB) ? (B - T) : curSize.y;
	if (w < 0) w = 0;
	if (h < 0) h = 0;

	const int left = haveL ? L : haveR ? (R - w) : haveCX ? (CX - w / 2) : (ownerTL.x + obj->getOffset().x);
	const int top  = haveT ? T : haveB ? (B - h) : haveCY ? (CY - h / 2) : (ownerTL.y + obj->getOffset().y);

	if ((haveL && haveR) || (haveT && haveB))
		setSize(handle, static_cast<float>(w), static_cast<float>(h));   // two-point stretch derives size

	if (owner && owner != this)
		obj->setAnchor(&owner->getTopLeftCornerRef());
	obj->setOffset({ left - ownerTL.x, top - ownerTL.y });
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
	m_inputState.clear();
	m_mouseQueue.clear();
	m_menuResults.clear();
	m_anchors.clear();
	m_dynamicAnchored.clear();
	m_parent.clear();
	m_name.clear();
	m_dragHandle = 0;
	m_nextHandle = 100000;
}

// Recompute every dynamic (frame-relative / two-point) anchor just before drawing, so frames track their
// relatives. Static single-point-to-owner frames were applied once at setPoint and skip this pass.
void LuaFrameManager::render()
{
	for (int h : m_dynamicAnchored)
		relayout(h);
	RenderObjectHolder::render();
}

// ---- introspection / visual primitives ----

int LuaFrameManager::parentOf(int handle) const { auto it = m_parent.find(handle); return it != m_parent.end() ? it->second : 0; }
void LuaFrameManager::setName(int handle, const std::string& name) { if (lookup(handle) && !name.empty()) m_name[handle] = name; }
std::string LuaFrameManager::frameName(int handle) const { auto it = m_name.find(handle); return it != m_name.end() ? it->second : std::string(); }

bool LuaFrameManager::isVisible(int handle) const
{
	auto* o = lookup(handle);
	if (!o)
		return false;
	for (RenderObject* p = o; p != nullptr; p = p->getOwner())
	{
		if (p->isHidden() || p->isForceHidden())
			return false;
		if (p->getOwner() == p)
			break;
	}
	return true;
}

std::string LuaFrameManager::objectType(int handle) const
{
	auto* o = lookup(handle);
	if (dynamic_cast<LuaButton*>(o))     return "Button";
	if (dynamic_cast<LuaStatusBar*>(o))  return "StatusBar";
	if (dynamic_cast<LuaEditBox*>(o))    return "EditBox";
	if (dynamic_cast<LuaTexture*>(o))    return "Texture";
	if (dynamic_cast<LuaFontString*>(o)) return "FontString";
	if (dynamic_cast<LuaFrame*>(o))      return "Frame";
	return "";
}

void LuaFrameManager::setAlpha(int handle, float a)
{
	const int ai = std::max(0, std::min(255, static_cast<int>(a * 255.f)));
	auto* o = lookup(handle);
	if (auto t = dynamic_cast<LuaTexture*>(o))          t->setAlpha(ai);
	else if (auto s = dynamic_cast<LuaStatusBar*>(o))   s->setAlpha(ai);
	else if (auto fs = dynamic_cast<LuaFontString*>(o)) fs->setAlpha(ai);
	// frames have no own visual; alpha is per-region in v1 (no child propagation)
}

void LuaFrameManager::setTexCoord(int handle, float l, float r, float t, float b)
{
	if (auto tex = dynamic_cast<LuaTexture*>(lookup(handle))) tex->setTexCoord(l, r, t, b);
}

// ---- widget interaction state ----

void LuaFrameManager::setMouseEnabled(int handle, bool v) { if (lookup(handle)) m_inputState[handle].mouseEnabled = v; }
void LuaFrameManager::setMovable(int handle, bool v)      { if (lookup(handle)) m_inputState[handle].movable = v; }
void LuaFrameManager::setDragButton(int handle, int btn)  { if (lookup(handle)) m_inputState[handle].dragButton = btn; }

bool LuaFrameManager::popMouseEvent(LuaUI::WidgetMouseEvent& out)
{
	if (m_mouseQueue.empty())
		return false;
	out = m_mouseQueue.front();
	m_mouseQueue.erase(m_mouseQueue.begin());
	return true;
}

bool LuaFrameManager::isShownSelf(int handle) const
{
	auto* o = lookup(handle);
	return o && !o->isHidden();
}

float LuaFrameManager::statusBarValue(int handle) const { auto* sb = dynamic_cast<LuaStatusBar*>(lookup(handle)); return sb ? sb->value() : 0.f; }
float LuaFrameManager::statusBarMin(int handle) const   { auto* sb = dynamic_cast<LuaStatusBar*>(lookup(handle)); return sb ? sb->minValue() : 0.f; }
float LuaFrameManager::statusBarMax(int handle) const   { auto* sb = dynamic_cast<LuaStatusBar*>(lookup(handle)); return sb ? sb->maxValue() : 0.f; }

sf::Vector2i LuaFrameManager::frameSize(int handle) const
{
	auto* o = lookup(handle);
	return o ? sizeOf(o) : sf::Vector2i{ 0, 0 };
}

int LuaFrameManager::topMouseHandleAt(sf::Vector2i p) const
{
	int best = 0, bestLevel = INT_MIN;
	for (auto& [h, st] : m_inputState)
	{
		if (!st.mouseEnabled || !hitTest(h, p))
			continue;
		auto* o = lookup(h);
		const int level = o ? o->getZLevel() : 0;
		if (level >= bestLevel) { bestLevel = level; best = h; }
	}
	return best;
}

// Detect+consume mouse buttons/wheel for the topmost mouse-enabled Lua frame under the cursor, and run the
// automatic drag state machine. Runs in the input pass (before the world's click-to-move), so consuming the
// event (clearMouseDown/Up/Wheel) prevents click-through. Detected events are queued for LuaEngine::onFrame.
void LuaFrameManager::input()
{
	RenderObjectHolder::input();   // children first (buttons set their own m_clicked, editboxes focus, etc.)

	const sf::Vector2i mp = sApplication->mousePos();

	// --- drag follow / stop (runs before new-press handling so a release ends the drag cleanly) ---
	if (m_dragHandle)
	{
		auto it = m_inputState.find(m_dragHandle);
		const int btn = (it != m_inputState.end()) ? it->second.dragButton : 0;
		const auto sfBtn = static_cast<sf::Mouse::Button>(btn < 0 ? 0 : btn);
		if (sf::Mouse::isButtonPressed(sfBtn))
		{
			if (auto* o = lookup(m_dragHandle))
			{
				const sf::Vector2i ownerTL = o->getOwner() ? o->getOwner()->getTopLeftCornerRef() : sf::Vector2i{ 0, 0 };
				o->setOffset({ mp.x - m_dragGrab.x - ownerTL.x, mp.y - m_dragGrab.y - ownerTL.y });
			}
		}
		else
		{
			m_mouseQueue.push_back({ m_dragHandle, LuaUI::WE_DragStop, btn, 0.f });
			const int drop = topMouseHandleAt(mp);
			if (drop && drop != m_dragHandle)
				m_mouseQueue.push_back({ drop, LuaUI::WE_ReceiveDrag, btn, 0.f });
			m_dragHandle = 0;
		}
	}

	const int hit = topMouseHandleAt(mp);

	// --- mouse buttons: left/right/middle, press + release ---
	static const sf::Mouse::Button kButtons[] = { sf::Mouse::Left, sf::Mouse::Right, sf::Mouse::Middle };
	for (int i = 0; i < 3; ++i)
	{
		const sf::Mouse::Button b = kButtons[i];
		if (hit && sApplication->mouseDown(b))
		{
			m_mouseQueue.push_back({ hit, LuaUI::WE_MouseDown, i, 0.f });

			// Begin a drag if this frame is movable and registered for this button.
			auto it = m_inputState.find(hit);
			if (!m_dragHandle && it != m_inputState.end() && it->second.movable && it->second.dragButton == b)
			{
				if (auto* o = lookup(hit))
				{
					const auto& tl = o->getTopLeftCornerRef();
					m_dragHandle = hit;
					m_dragGrab = { mp.x - tl.x, mp.y - tl.y };
					clearAllPoints(hit);   // become offset-positioned so relayout won't fight the drag
					m_mouseQueue.push_back({ hit, LuaUI::WE_DragStart, i, 0.f });
				}
			}
			sApplication->clearMouseDown();   // consume so the world doesn't also act on it
		}
		if (hit && sApplication->mouseUp(b))
		{
			m_mouseQueue.push_back({ hit, LuaUI::WE_MouseUp, i, 0.f });
			sApplication->clearMouseUp();
		}
	}

	// --- mouse wheel ---
	if (hit)
	{
		const float delta = sApplication->getMouseWheelEvent().delta;
		if (delta != 0.f)
		{
			m_mouseQueue.push_back({ hit, LuaUI::WE_MouseWheel, 0, delta });
			sApplication->clearMouseWheelEvent();
		}
	}
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

	void setAllPoints(int handle, int relHandle) { if (auto* m = LuaFrameManager::instance()) m->setAllPoints(handle, relHandle); }
	void clearAllPoints(int handle)              { if (auto* m = LuaFrameManager::instance()) m->clearAllPoints(handle); }

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

	// The live C++ UnitFrame for a token (player/target/party1..3). These frames are force-hidden in favor of
	// the Lua replacement but still alive, so they own the resolved unit + portrait + the context menu.
	static UnitFrame* resolveUnitFrame(const std::string& token)
	{
		auto* w = currentWorld();
		if (!w)
			return nullptr;
		const int id = (token == "player")  ? World::PlayerUnitFrame
		             : (token == "target")  ? World::TargetUnitFrame
		             : (token == "party1")  ? World::Party1UnitFrame
		             : (token == "party2")  ? World::Party2UnitFrame
		             : (token == "party3")  ? World::Party3UnitFrame : 0;
		if (!id)
			return nullptr;
		return dynamic_cast<UnitFrame*>(w->getRenderObject(id).get());
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
		if (auto* f = resolveUnitFrame(token))   // party1..3 via the live C++ frame's unit
			return f->getUnitPtr();
		return nullptr;
	}

	int unitHealth(const std::string& token)    { auto* u = resolveUnit(token); return u ? u->getHealth() : 0; }
	int unitHealthMax(const std::string& token) { auto* u = resolveUnit(token); return u ? u->getMaxHealth() : 0; }
	int unitLevel(const std::string& token)     { auto* u = resolveUnit(token); return u ? u->getLevel() : 0; }
	int unitPower(const std::string& token)     { auto* u = resolveUnit(token); return u ? u->getMana() : 0; }
	int unitPowerMax(const std::string& token)  { auto* u = resolveUnit(token); return u ? u->getMaxMana() : 0; }
	std::string unitName(const std::string& token) { auto* u = resolveUnit(token); return u ? u->getName() : std::string(); }
	bool unitExists(const std::string& token)   { return resolveUnit(token) != nullptr; }

	int unitNameColor(const std::string& token)
	{
		auto* u = resolveUnit(token);
		if (!u) return 0xFFFFFF;
		const sf::Color c = u->getNameColor();
		return (int(c.r) << 16) | (int(c.g) << 8) | int(c.b);
	}

	int unitFlag(const std::string& token, const std::string& name)
	{
		auto* u = resolveUnit(token);
		if (!u) return 0;
		// Elite/Boss come from the NPC template (bool_elite/bool_boss), not an object variable.
		if (name == "Elite") return atoi(sContentMgr->db("npc_template").data(u->getEntry(), "bool_elite").c_str());
		if (name == "Boss")  return atoi(sContentMgr->db("npc_template").data(u->getEntry(), "bool_boss").c_str());
		const ObjDefines::Variable v =
			  (name == "InCombat")      ? ObjDefines::Variable::InCombat
			: (name == "InArenaQueue")  ? ObjDefines::Variable::InArenaQueue
			: (name == "DynGreyTagged") ? ObjDefines::Variable::DynGreyTagged
			: ObjDefines::Variable::Health;   // unknown name => harmless read
		return u->getVariable(v);
	}

	bool unitIsDead(const std::string& token)   { auto* u = resolveUnit(token); return u && u->getHealth() <= 0; }
	bool unitIsPlayer(const std::string& token) { auto* u = resolveUnit(token); return u && u->getType() == MutualObject::Type::Player; }

	bool unitIsPartyLeader(const std::string& token)
	{
		auto* w = currentWorld();
		auto* u = resolveUnit(token);
		return w && u && w->isPartyLeader(u->getGuid());
	}

	bool unitHasBrokenEquipment(const std::string& token)
	{
		auto* p = dynamic_cast<ClientPlayer*>(resolveUnit(token));
		return p && p->hasBrokenEquipment();
	}

	std::string unitPortraitTexture(const std::string& token)
	{
		auto* f = resolveUnitFrame(token);
		if (!f) return std::string();
		auto* spr = f->getPortraitPtr();
		return spr ? spr->getTextureName() : std::string();
	}

	int unitCastSpell(const std::string& token) { auto* u = resolveUnit(token); return u ? u->getCastSpellId() : 0; }

	int unitCastElapsedMs(const std::string& token)
	{
		auto* u = resolveUnit(token);
		if (!u || u->getCastSpellId() == 0) return 0;
		return static_cast<int>(std::clock()) - static_cast<int>(u->getCastStartTime());
	}

	int unitCastTotalMs(const std::string& token)
	{
		auto* u = resolveUnit(token);
		if (!u || u->getCastSpellId() == 0) return 0;
		return static_cast<int>(u->getCastStopTime()) - static_cast<int>(u->getCastStartTime());
	}

	int unitAuraCount(const std::string& token, bool harmful)
	{
		auto* u = resolveUnit(token);
		if (!u) return 0;
		return static_cast<int>((harmful ? u->getDebuffs() : u->getBuffs()).size());
	}

	bool unitAura(const std::string& token, int index, bool harmful, int& spellId, int& count, int& remainingMs, int& durationMs)
	{
		auto* u = resolveUnit(token);
		if (!u) return false;
		const auto& list = harmful ? u->getDebuffs() : u->getBuffs();
		if (index < 0 || index >= static_cast<int>(list.size())) return false;
		const auto& a = list[index];
		spellId    = a.spellId;
		count      = a.stackCount;
		durationMs = a.maxDuration;
		remainingMs = static_cast<int>(a.endDate - sApplication->timeNowMs());   // 0/neg => expired/permanent
		if (a.maxDuration == 0) remainingMs = 0;
		return true;
	}

	bool partyMemberExists(int idx)
	{
		auto* w = currentWorld();
		if (!w) return false;
		const int id = (idx == 0) ? World::Party1UnitFrame : (idx == 1) ? World::Party2UnitFrame : (idx == 2) ? World::Party3UnitFrame : 0;
		if (!id) return false;
		auto* f = dynamic_cast<UnitFrame*>(w->getRenderObject(id).get());
		return f && f->getUnitPtr() != nullptr;
	}

	void unitContextMenu(const std::string& token, const std::string& extraLines)
	{
		auto* f = resolveUnitFrame(token);
		if (!f) return;
		std::vector<std::string> extra;
		for (size_t i = 0; i < extraLines.size(); )
		{
			const size_t nl = extraLines.find('\n', i);
			const std::string line = extraLines.substr(i, nl == std::string::npos ? std::string::npos : nl - i);
			if (!line.empty()) extra.push_back(line);
			if (nl == std::string::npos) break;
			i = nl + 1;
		}
		f->openContextMenu(LuaFrameManager::instance(), extra);
	}

	bool popMenuResult(std::string& out) { auto* m = LuaFrameManager::instance(); return m && m->popMenuResult(out); }

	void showUnitTooltip(const std::string& token) { if (auto* u = resolveUnit(token)) u->useTooltip(); }

	void showSpellTooltip(int spellId)
	{
		auto* w = currentWorld();
		if (!w || spellId == 0)
			return;
		auto& st = sContentMgr->db("spell_template");
		auto tip = make_shared<Tooltip>(*w, sf::Vector2i{});
		tip->setShadowOffset(1);
		tip->addLine(Assets::FontId::Arial, 15, st.data(spellId, "name"), sf::Color(240, 197, 2, 255));
		const std::string desc = st.data(spellId, "aura_description");
		if (!desc.empty())
			tip->addLine(Assets::FontId::Arial, 15, desc);
		const auto mp = sApplication->mousePos();
		tip->moveTo({ mp.x + 16, mp.y + 16 });
		sApplication->setTooltip(tip);
	}

	void saveUISetting(const std::string& key, int value) { sConfig->setInt("UI", key.c_str(), value); }
	int  getUISetting(const std::string& key, int def)    { return sConfig->getInt("UI", key.c_str(), def); }

	std::string spellTexture(int spellId) { return sContentMgr->db("spell_template").data(spellId, "icon"); }
	std::string spellName(int spellId)    { return sContentMgr->db("spell_template").data(spellId, "name"); }

	int textureWidth(const std::string& name)  { auto t = sContentMgr->getTexture(name); return t ? static_cast<int>(t->getSize().x) : 0; }
	int textureHeight(const std::string& name) { auto t = sContentMgr->getTexture(name); return t ? static_cast<int>(t->getSize().y) : 0; }

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
		             : (name == "Party1Frame") ? World::Party1UnitFrame
		             : (name == "Party2Frame") ? World::Party2UnitFrame
		             : (name == "Party3Frame") ? World::Party3UnitFrame
		             : (name == "CastBar")     ? World::PlayerCastBar
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

	bool isInWorld() { auto* w = currentWorld(); return w && w->myself(); }

	bool isMouseOver(int handle)
	{
		auto* m = LuaFrameManager::instance();
		return m && m->hitTest(handle, sApplication->mousePos());
	}

	// ---- widget interaction (forward to the manager) ----
	bool popMouseEvent(WidgetMouseEvent& out)        { auto* m = LuaFrameManager::instance(); return m && m->popMouseEvent(out); }
	void setMouseEnabled(int handle, bool v)         { if (auto* m = LuaFrameManager::instance()) m->setMouseEnabled(handle, v); }
	void setMovable(int handle, bool v)              { if (auto* m = LuaFrameManager::instance()) m->setMovable(handle, v); }
	void setDragButton(int handle, int sfButton)     { if (auto* m = LuaFrameManager::instance()) m->setDragButton(handle, sfButton); }
	bool  isShownSelf(int handle)                    { auto* m = LuaFrameManager::instance(); return m && m->isShownSelf(handle); }
	float statusBarValue(int handle)                 { auto* m = LuaFrameManager::instance(); return m ? m->statusBarValue(handle) : 0.f; }
	float statusBarMin(int handle)                   { auto* m = LuaFrameManager::instance(); return m ? m->statusBarMin(handle) : 0.f; }
	float statusBarMax(int handle)                   { auto* m = LuaFrameManager::instance(); return m ? m->statusBarMax(handle) : 0.f; }
	int   frameWidth(int handle)                     { auto* m = LuaFrameManager::instance(); return m ? m->frameSize(handle).x : 0; }
	int   frameHeight(int handle)                    { auto* m = LuaFrameManager::instance(); return m ? m->frameSize(handle).y : 0; }
	int   frameLeft(int handle)                      { auto* m = LuaFrameManager::instance(); auto* o = m ? m->lookup(handle) : nullptr; return o ? o->getTopLeftCornerRef().x : 0; }
	int   frameTop(int handle)                       { auto* m = LuaFrameManager::instance(); auto* o = m ? m->lookup(handle) : nullptr; return o ? o->getTopLeftCornerRef().y : 0; }
	int   parentOf(int handle)                       { auto* m = LuaFrameManager::instance(); return m ? m->parentOf(handle) : 0; }
	void  setName(int handle, const std::string& n)  { if (auto* m = LuaFrameManager::instance()) m->setName(handle, n); }
	std::string frameName(int handle)                { auto* m = LuaFrameManager::instance(); return m ? m->frameName(handle) : std::string(); }
	bool  isVisible(int handle)                      { auto* m = LuaFrameManager::instance(); return m && m->isVisible(handle); }
	std::string objectType(int handle)               { auto* m = LuaFrameManager::instance(); return m ? m->objectType(handle) : std::string(); }
	void  setAlpha(int handle, float a)              { if (auto* m = LuaFrameManager::instance()) m->setAlpha(handle, a); }
	void  setTexCoord(int handle, float l, float r, float t, float b) { if (auto* m = LuaFrameManager::instance()) m->setTexCoord(handle, l, r, t, b); }
	void  setTextureCircle(int handle, int radius) { auto* m = LuaFrameManager::instance(); if (auto t = dynamic_cast<LuaTexture*>(m ? m->lookup(handle) : nullptr)) t->setCircle(radius); }

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
