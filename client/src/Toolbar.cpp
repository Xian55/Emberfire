#include "stdafx.h"
#include "Toolbar.h"
#include "SpriteRo.h"
#include "ContentMgr.h"
#include "SpellIcon.h"
#include "ItemIcon.h"
#include "SpriteRo.h"
#include "World.h"
#include "Application.h"
#include "Connector.h"
#include "ClientPlayer.h"
#include "ClientUnit.h"
#include "Abilities.h"
#include "Inventory.h"
#include "WorldSpellAnimation.h"
#include "Equipment.h"
#include "Options.h"
#include "Keybinds.h"
#include "Tooltip.h"

#include "..\Shared\Config.h"
#include "..\Shared\StlBuffer.h"
#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\SpellDefines.h"
#include "..\Shared\GamePacketClient.h"

#include <filesystem>

Toolbar::Toolbar(World& owner, const int id, const sf::Vector2i topLeftCorner) :
	WorldChild(owner, id, owner)
{
	setMultiInput(true);

	getTopLeftCornerRef() = topLeftCorner;
	updateBottomRight();

	createBaseIcon(GameIcon1, GameIcon::Type::Spell, 0);
	createBaseIcon(GameIcon2, GameIcon::Type::Spell, 0);
	createBaseIcon(GameIcon3, GameIcon::Type::Spell, 0);
	createBaseIcon(GameIcon4, GameIcon::Type::Spell, 0);
	createBaseIcon(GameIcon5, GameIcon::Type::Spell, 0);
	createBaseIcon(GameIcon6, GameIcon::Type::Spell, 0);
	createBaseIcon(GameIcon7, GameIcon::Type::Spell, 0);
	createBaseIcon(GameIcon8, GameIcon::Type::Spell, 0);
	createBaseIcon(GameIcon9, GameIcon::Type::Spell, 0);
	createBaseIcon(GameIcon10, GameIcon::Type::Spell, 0);
	createBaseIcon(GameIcon11, GameIcon::Type::Spell, 0);
	createBaseIcon(GameIcon12, GameIcon::Type::Spell, 0);
}

Toolbar::~Toolbar()
{

}

void Toolbar::input()
{
	// Lua view: the addon owns visuals/clicks/drag. We still run the icons' input (so keybinds are detected,
	// keyboard-global — independent of mouse) and fire only the keybound activation; everything else is skipped.
	if (m_luaView)
	{
		__super::input();

		if (auto useButton = popKeyboundButton())
			if (auto gameIcon = dynamic_pointer_cast<GameIcon>(useButton))
				useIcon(static_cast<Interface>(gameIcon->getId()));

		return;
	}

	// The "bar" is not "clickable" unless an icon is held
	if (world().isIconGrabbed())
		updateBottomRight();
	else
		getBottomRightCornerRef() = getTopLeftCornerRef();

	__super::input();

	// Must come before grabIcon otherwise key bind presses will result in a grab
	auto useButton = popKeyboundButton();

	if (!world().isIconGrabbed())
	{
		if (useButton == nullptr)
			useButton = popFirstLeftClickButton();

		if (useButton == nullptr)
			useButton = popFirstRightClickButton();
	}

	if (auto gameIcon = dynamic_pointer_cast<GameIcon>(useButton))
	{
		useIcon(static_cast<Interface>(gameIcon->getId()));
	}
	else if (!__super::grabIcon() && __super::depositGrabbedIconToEmptySpace())
	{
		sContentMgr->playSound(SfxId::WindowTargetClose);
		world().getGrabbedIcon()->changeEntry(0);
		world().setGrabbedIcon(nullptr);
		saveCache();
	}
}

void Toolbar::setLuaView(const bool v)
{
	m_luaView = v;

	// The icons must NOT consume mouse clicks in Lua-view: the C++ icons sit under the Lua ActionBar buttons,
	// and Button::updatePressed calls clearMouseDown/Up on a moused-over click, eating it before the Lua frame
	// manager (= no pickup / click-cast / void-drop on the Lua buttons). Keybind detection is a separate path
	// in Button::input and keeps working. createBaseIcon re-applies this for any slot reassigned while in view.
	for (auto& itr : m_icons)
	{
		itr.second->setAllowLeftClick(!v);
		itr.second->setAllowRightClick(!v);
	}
}

void Toolbar::useIcon(const Interface id)
{
	auto gameIcon = dynamic_pointer_cast<GameIcon>(getRenderObject(id));

	if (gameIcon == nullptr || gameIcon->getEntry() == 0)
		return;

	gameIcon->setExclaimNotice(false);

	if (gameIcon->getType() == GameIcon::Type::Spell)
	{
		castSpell(gameIcon->getEntry());
	}
	else if (gameIcon->getType() == GameIcon::Type::Item)
	{
		auto inv = dynamic_pointer_cast<Inventory>(world().getRenderObject(World::Interface::InventoryPanel));
		int slot = inv->getFirstItemEntrySlot(gameIcon->getEntry());

		if (slot >= 0)
		{
			GP_Client_UseItem packet;
			packet.m_slot = slot;
			packet.m_itemId = gameIcon->getEntry();
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}
	}
}

void Toolbar::render()
{
	if (m_luaView)
		return;   // Lua ActionBar draws the icons; the C++ bar is data/cast only

	const bool darkenAll = !world().canAct();
	shared_ptr<ClientUnit> selectedUnit = dynamic_pointer_cast<ClientUnit>(world().getClientObject(world().getSelectedGuid()));
	int distanceToTarget = 0;

	if (selectedUnit != nullptr)
	{
		// Use the player's CURRENT position (same space as the target). The server
		// position isn't echoed for self (no op90), so getServerPosition() stays stale
		// at the origin and would make every target read as out of range.
		Geo2d::Vector2 myself = world().myself()->computeRawScreenPosition();
		sf::Vector2f target = sf::Vector2f(selectedUnit->computeRawScreenPositioni());
		distanceToTarget = int(myself.getDist({ target.x, target.y }));
	}
		
	auto inventory = dynamic_pointer_cast<Inventory>(world().getRenderObject(World::Interface::InventoryPanel));
	auto abilities = dynamic_pointer_cast<Abilities>(world().getRenderObject(World::Interface::AbilitiesPanel));

	for (auto& ref : m_icons)
	{
		auto& itr = ref.second;
		bool darkenItr = darkenAll || (itr->getType() == GameIcon::Spell && shouldDarkenSpellId(itr->getEntry()));
		
		itr->setHidden(itr->getEntry() == 0 && !world().isIconGrabbed());
		itr->setDarken(darkenItr);
		itr->setBindColor(sf::Color::White);
		itr->setOom(false);

		auto cd = [&](const int id)
		{
			if (id == 0)
				return;

			// 0, 0 will just mean no cooldown
			auto cd = world().getCooldown(id);		
			itr->setCooldown(cd.first, cd.second);
		};
		
		cd(itr->getCastedSpellId());
		cd(-itr->getCastedSpellCategoryCooldown());
		
		// Out of range color
		if (selectedUnit != nullptr && itr->getType() == GameIcon::Spell)
		{
			if (itr->getSpellRange() > 0 && distanceToTarget > itr->getSpellRange())
				itr->setBindColor(sf::Color::Red);

			if (itr->getSpellRangeMin() > 0 && distanceToTarget < itr->getSpellRangeMin())
				itr->setBindColor(sf::Color::Red);
		}

		// Out of mana color
		if (world().myself() != nullptr && itr->getSpellManaCost(*world().myself()) > world().myself()->getMana())
			itr->setOom(true);

		if (itr->getEntry() != 0 && !itr->isHidden())
			updateInvSpbookCheck(*itr, *inventory, *abilities);

		if (itr->getType() == GameIcon::Spell && itr->isHeldDown())
		{
			const auto& cacheItr = m_spellPreloadCache[itr->getId()];
			
			if (expiredPreload(cacheItr))
			{
				// Pre load the animation since there's a high chance we'll use them once the player is at the point where he's thinking of casting the spell				
				vector<shared_ptr<SpriteAnimation>> objs;
				WorldSpellAnimation::preLoad(itr->getEntry(), objs);
				m_spellPreloadCache[itr->getId()] = { std::clock(), objs };
			}
		}
	}

	for (auto itr = m_spellPreloadCache.begin(); itr != m_spellPreloadCache.end();)
	{
		if (expiredPreload(itr->second))
		{
			itr->second.second.clear();
			itr = m_spellPreloadCache.erase(itr);
		}
		else
		{
			++itr;
		}

	}

	__super::render();
}

bool Toolbar::shouldDarkenSpellId(const int spellId) const
{
	if (world().myself() == nullptr)
		return false;

	// While casting in progress, that icon is darkened
	if (spellId == world().myself()->getCastSpellId())
		return true;
	
	// While selecting target location, every other spell is hidden
	if (world().getGroundActionSpell() != 0 && spellId != world().getGroundActionSpell())
		return true;
	
	auto& db = sContentMgr->db("spell_template");
	int req_caster_mechanic = atoi(db.data(spellId, "req_caster_mechanic").c_str());
	int req_tar_mechanic = atoi(db.data(spellId, "req_tar_mechanic").c_str());
	int req_caster_aura = atoi(db.data(spellId, "req_caster_aura").c_str());
	int req_tar_aura = atoi(db.data(spellId, "req_tar_aura").c_str());

	if (req_caster_mechanic != 0 && (world().myself()->getVariable(ObjDefines::Variable::MechanicMask) & req_caster_mechanic) == 0)
		return true;

	if (req_caster_aura != 0 && !world().myself()->hasAura(req_caster_aura))
		return true;

	auto targetPtr = world().selectedUnit();

	if (targetPtr == nullptr)
		targetPtr = world().myself();

	if (targetPtr != nullptr)
	{
		if (req_tar_mechanic != 0 && (targetPtr->getVariable(ObjDefines::Variable::MechanicMask) & req_tar_mechanic) == 0)
			return true;

		if (req_tar_aura != 0 && !targetPtr->hasAura(req_tar_aura))
			return true;
	}

	return false;
}

bool Toolbar::expiredPreload(const pair<clock_t, vector<shared_ptr<SpriteAnimation>>>& dat) const
{
	return std::clock() - dat.first > 10000;
}

void Toolbar::updateBottomRight()
{
	// 40x40 sized buttons
	getBottomRightCornerRef() = getTopLeftCornerRef() + sf::Vector2i(40 * 12, 40);
}
		
void Toolbar::updateInvSpbookCheck(GameIcon& icon, Inventory& inv, Abilities& spells)
{
	int stack = 0;

	if (icon.getType() == GameIcon::Item)
	{
		if (!inv.getItemStack(icon.getEntry(), stack))
			icon.setDarken(true);
	}
	else if (icon.getType() == GameIcon::Spell)
	{
		GP_Server_Spellbook::SpellSlot slot;

		if (spells.getSpellPoints(icon.getEntry(), slot))
		{
			bool changeA = icon.setBasePoints(slot.bpoints);
			bool changeB = icon.setLevel(slot.level);

			if (changeA || changeB)
			{
				int entry = icon.getEntry();
				icon.changeEntry(0);
				icon.changeEntry(entry);
			}
		}
		else
		{
			icon.changeEntry(0);
		}
	}
		
	icon.setRenderStackAtBottom(true);
	icon.setStackCount(stack);
}

void Toolbar::castSpell(int spellId)
{
	auto& st = sContentMgr->db("spell_template");
	uint64_t attributes = _atoi64(st.data(spellId, "attributes").c_str());

	if (Util::maskHas(attributes, SpellDefines::Attributes::TargetsGround) || Util::maskHas(attributes, SpellDefines::Attributes::MouseoverTargeting))
	{
		world().setGroundActionSpell(spellId);
		return;
	}

	world().setGroundActionSpell(0);

	// This doesn't make any rational sense, I just think it makes the game more fun
	if (spellId == SpellDefines::StaticSpells::MeleeSpell ||  spellId == SpellDefines::StaticSpells::RangedSpell)
	{
		if (auto ptr = world().selectedUnit())
		{
			if (ptr->getVariable(ObjDefines::Variable::DynLootable))
				spellId = SpellDefines::StaticSpells::LootUnit;
		}
	}

	GP_Client_CastSpell packet;
	packet.m_spellId = spellId;
	packet.m_targetGuid = world().getSelectedGuid();
	packet.m_targetPosX = packet.m_targetPosY = 0;
	packet.m_ctm = sConfig->getCache(Options::System_CtmTick);
	sConnector->sendPacket(packet.build(StlBuffer{}));
}

void Toolbar::refreshTooltips()
{
	for (auto& itr : m_icons)
		itr.second->refreshTooltip();
}
		
void Toolbar::assignIcon(const Interface id, const int type, const int entry)
{
	// Preserve the slot's keybind across the icon recreate (createBaseIcon makes a fresh GameIcon).
	SfKeyEvent ke;

	if (auto old = dynamic_pointer_cast<GameIcon>(getRenderObject(id)))
		ke = old->getKeyEvent();

	createBaseIcon(id, type, entry);
	setButtonBind(id, ke);
	saveCache();
}

bool Toolbar::iconInfo(const Interface id, int& type, int& entry, string& texture) const
{
	auto icon = dynamic_pointer_cast<GameIcon>(getRenderObject(id));

	if (icon == nullptr)
		return false;

	type = icon->getType();
	entry = icon->getEntry();
	texture.clear();

	if (entry != 0)
	{
		const char* table = (type == GameIcon::Type::Spell) ? "spell_template" : "item_template";
		texture = sContentMgr->db(table).data(entry, "icon");
	}

	return true;
}

bool Toolbar::iconCooldown(const Interface id, int& remainingMs, int& durationMs) const
{
	auto icon = dynamic_pointer_cast<GameIcon>(getRenderObject(id));

	if (icon == nullptr || icon->getEntry() == 0)
		return false;

	auto pick = [&](const int cid) -> CooldownHolder::CooldownPair
	{
		return cid == 0 ? CooldownHolder::CooldownPair{ 0, 0 } : world().getCooldown(cid);
	};

	// The spell's own cooldown and its category cooldown (negative key) — show whichever ends later.
	auto a = pick(icon->getCastedSpellId());
	auto b = pick(-icon->getCastedSpellCategoryCooldown());
	auto cd = (b.second > a.second) ? b : a;

	const __time64_t now = sApplication->timeNowMs();

	if (cd.second <= now)
		return false;

	remainingMs = static_cast<int>(cd.second - now);
	durationMs = static_cast<int>(cd.second - cd.first);
	return true;
}

int Toolbar::iconStateFlags(const Interface id) const
{
	auto icon = dynamic_pointer_cast<GameIcon>(getRenderObject(id));

	if (icon == nullptr || icon->getEntry() == 0)
		return 0;

	int flags = 1;   // has entry

	if (!world().canAct() || (icon->getType() == GameIcon::Spell && shouldDarkenSpellId(icon->getEntry())))
		flags |= 2;

	// Out of range vs the current target (same math as render()).
	if (icon->getType() == GameIcon::Spell && world().myself() != nullptr)
	{
		if (auto sel = dynamic_pointer_cast<ClientUnit>(world().getClientObject(world().getSelectedGuid())))
		{
			Geo2d::Vector2 me = world().myself()->computeRawScreenPosition();
			sf::Vector2f tgt = sf::Vector2f(sel->computeRawScreenPositioni());
			const int dist = static_cast<int>(me.getDist({ tgt.x, tgt.y }));

			if (icon->getSpellRange() > 0 && dist > icon->getSpellRange())
				flags |= 4;

			if (icon->getSpellRangeMin() > 0 && dist < icon->getSpellRangeMin())
				flags |= 4;
		}
	}

	// Out of mana.
	if (world().myself() != nullptr && icon->getSpellManaCost(*world().myself()) > world().myself()->getMana())
		flags |= 8;

	return flags;
}

int Toolbar::iconStackCount(const Interface id) const
{
	auto icon = dynamic_pointer_cast<GameIcon>(getRenderObject(id));

	if (icon == nullptr || icon->getType() != GameIcon::Item)
		return 0;

	int stack = 0;

	if (auto inv = dynamic_pointer_cast<Inventory>(world().getRenderObject(World::Interface::InventoryPanel)))
		inv->getItemStack(icon->getEntry(), stack);

	return stack;
}

string Toolbar::iconKeybindText(const Interface id) const
{
	auto icon = dynamic_pointer_cast<GameIcon>(getRenderObject(id));

	if (icon == nullptr)
		return "";

	string s = sKeybinds->deduceKeybindStr(icon->getKeyEvent());
	return s == "null" ? string() : s;
}

shared_ptr<Tooltip> Toolbar::iconTooltip(const Interface id) const
{
	auto icon = dynamic_pointer_cast<GameIcon>(getRenderObject(id));
	return icon ? icon->rebuildTooltip() : nullptr;
}

shared_ptr<GameIcon> Toolbar::findIconByEntry(const int entry) const
{
	for (auto& itr : m_icons)
	{
		if (itr.second->getEntry() == entry)
			return itr.second;
	}

	return nullptr;
}
		
int Toolbar::getIconEntry(const Interface id) const
{
	if (auto icon = dynamic_pointer_cast<GameIcon>(getRenderObject(id)))
		return icon->getEntry();

	return 0;
}

void Toolbar::createBaseIcon(const Interface id, const int type, const int entry)
{
	shared_ptr<GameIcon> gameIcon;
	
	if (type == GameIcon::Type::Item)
		gameIcon = make_shared<ItemIcon>(*this, id, "gameicon40");
	else
		gameIcon = make_shared<SpellIcon>(*this, id, "gameicon40");

	gameIcon->setShowStackAmtThreshold(0);
	gameIcon->setOffset(sf::Vector2i((id - 1) * 46, 0));
	gameIcon->setAnchor(&getTopLeftCornerRef());
	gameIcon->setKeyDown(false);
	gameIcon->changeEntry(entry);
	gameIcon->setEnableDragActivate(true);
	gameIcon->setAllowRightClick(true);

	// In Lua-view the icons stay input-active (for keybinds) but must not consume mouse clicks — the Lua
	// ActionBar buttons do. (See setLuaView.)
	if (m_luaView)
	{
		gameIcon->setAllowLeftClick(false);
		gameIcon->setAllowRightClick(false);
	}

	addRenderObject(gameIcon);
	m_icons[id] = gameIcon;
}

void Toolbar::setButtonBind(const Interface id, const SfKeyEvent ke)
{
	if (auto gicon = dynamic_pointer_cast<GameIcon>(getRenderObject(id)))
		gicon->setKeyEvent(ke);
}

void Toolbar::givenGabbedIcon(shared_ptr<GameIcon> myIcon, shared_ptr<GameIcon> heldIcon)
{
	const auto heldType = heldIcon->getType();
	const auto heldEntry = heldIcon->getEntry();

	// Toolbar to Toolbar results in swapping values
	if (auto toolbar = dynamic_cast<Toolbar*>(heldIcon->getOwner()))
	{
		toolbar->createBaseIcon(static_cast<Interface>(heldIcon->getId()), myIcon->getType(), myIcon->getEntry());
		toolbar->saveCache();
	}
	
	createBaseIcon(static_cast<Interface>(myIcon->getId()), heldIcon->getType(), heldIcon->getEntry());
	saveCache();
	sContentMgr->playSound(SfxId::WindowTargetOpen);
}

string Toolbar::getCacheFilename() const
{
	return sApplication->getCacheFolderPath() + "toolbar_" + to_string(getId()) + "_" + m_playerName;
}

void Toolbar::saveCache()
{
	if (m_playerName.empty())
		return;
	
	StlBuffer buffer;
	buffer << static_cast<uint8_t>(Interface::GameIcon1);
	buffer << static_cast<uint8_t>(Interface::GameIcon12);

	for (int i = Interface::GameIcon1; i <= Interface::GameIcon12; ++i)
	{
		if (auto gicon = dynamic_pointer_cast<GameIcon>(getRenderObject(i)))
			buffer << static_cast<uint8_t>(gicon->getType()) << gicon->getEntry();
	}
	
	if (!filesystem::is_directory("cache") || !filesystem::exists("cache")) 
		filesystem::create_directory("cache");

	buffer.writeFile(getCacheFilename());
}

bool Toolbar::loadCache(const string& playerName)
{
	if (m_playerName == playerName)
		return false;

	m_playerName = playerName;
	
	StlBuffer buffer;

	if (!buffer.readFile(getCacheFilename()))
		return false;

	uint8_t start;
	uint8_t stop;

	buffer >> start;
	buffer >> stop;

	for (decltype(start) i = start; i <= stop; ++i)
	{
		uint8_t type;
		int entry;

		buffer >> type >> entry;
		
		// Verify that it's a GameIcon index
		if (auto gicon = dynamic_pointer_cast<GameIcon>(getRenderObject(i)))
			createBaseIcon(Interface(i), type, entry);
	}

	return true;
}