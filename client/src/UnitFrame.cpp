#include "stdafx.h"
#include "UnitFrame.h"
#include "Sprite.h"
#include "ContentMgr.h"
#include "TextBox.h"
#include "ClientNpc.h"
#include "ClientPlayer.h"
#include "Text.h"
#include "Application.h"
#include "World.h"
#include "SpellIcon.h"
#include "Connector.h"
#include "CastBar.h"
#include "ContentMgr.h"
#include "ParticleSystem.h"
#include "ConfirmMessageBox.h"
#include "GameChat.h"

#include "..\StringHelpers.h"
#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\GamePacketClient.h"

#include <assert.h>
#include <algorithm>

UnitFrame::UnitFrame(RenderObject& owner, const int id, const sf::Vector2i pos) :
	BuffDebuffRenderer(*dynamic_cast<World*>(&owner), &owner, id),
	m_levelFrameSprite(sContentMgr->spawnSprite("unit_frame_level_bg.png")),
	m_eliteSprite(sContentMgr->spawnSprite("unit_frame_elite.png")),
	m_bossSprite(sContentMgr->spawnSprite("unit_frame_boss.png")),
	m_combatSprite(sContentMgr->spawnSprite("unitframe_combat.png"))
{
	// Will be revealed when provided a unit
	setHidden(true);
	setFrameStyle(FrameStyle::LocalPlayer);
	setMultiInput(true);

	m_topLeftCorner = pos;

	m_lvlTxt = make_unique<TextBox>(sContentMgr->getFont(FontId::Palatino), 12);
	m_lvlTxt->setColor(sf::Color(132, 116, 89, 255));

	m_nameTxt = make_unique<Text>(sContentMgr->getFont(FontId::PalatinoBold));
	m_nameTxt->setCharacterSize(14);
	//m_nameTxt->setOriginalColor(sf::Color(134, 115, 92, 255));
	m_nameTxt->setOutlineThickness(2.0f);
	m_nameTxt->setOutlineColor(sf::Color(0, 0, 0, 63));

	m_manaTxt = make_unique<Text>(sContentMgr->getFont(FontId::Palatino));
	m_manaTxt->setCharacterSize(12);
	m_manaTxt->setOriginalColor(sf::Color(147, 129, 114, 255));
	m_manaTxt->setOutlineThickness(2.0f);
	m_manaTxt->setOutlineColor(sf::Color(0, 0, 0, 63));

	m_hpTxt = make_unique<Text>(sContentMgr->getFont(FontId::Palatino));
	m_hpTxt->setCharacterSize(12);
	m_hpTxt->setOriginalColor(sf::Color(147, 129, 114, 255));
	m_hpTxt->setOutlineThickness(2.0f);
	m_hpTxt->setOutlineColor(sf::Color(0, 0, 0, 63));

	m_hpPctTxt = make_unique<Text>(sContentMgr->getFont(FontId::Palatino));
	m_hpPctTxt->setCharacterSize(12);
	m_hpPctTxt->setOriginalColor(sf::Color(147, 129, 114, 255));
	m_hpPctTxt->setOutlineThickness(2.0f);
	m_hpPctTxt->setOutlineColor(sf::Color(0, 0, 0, 63));

	m_manaPctTxt = make_unique<Text>(sContentMgr->getFont(FontId::Palatino));
	m_manaPctTxt->setCharacterSize(12);
	m_manaPctTxt->setOriginalColor(sf::Color(147, 129, 114, 255));
	m_manaPctTxt->setOutlineThickness(2.0f);
	m_manaPctTxt->setOutlineColor(sf::Color(0, 0, 0, 63));

	m_combatMsgTxt = make_unique<Text>(sContentMgr->getFont(FontId::Ringbearer));
	m_combatMsgTxt->setCharacterSize(56);
	m_combatMsgTxt->setShadowOffset(1);

	m_partyLeaderSprite = sContentMgr->spawnSprite("unitframe_party_leader.png");
	m_partyLeaderSprite->setHotspotEasy(true, true);

	m_criticalStatusPsi = sContentMgr->spawnParticleSystem("repair_red.psi");
}

UnitFrame::~UnitFrame()
{

}

void UnitFrame::input()
{
	__super::input();

	m_bottomRightCorner = m_topLeftCorner + sf::Vector2i(m_frameSprite->getTexture()->getSize());

	BuffDebuffRenderer::processRightClickedId(popFirstRightClickButtonId());

	if (sApplication->mouseUp(sf::Mouse::Left) && isMousedOver())
	{
		if (m_unit != nullptr)
		{
			if (auto owner = dynamic_cast<World*>(getOwner()))
				owner->setSelectedGuid(m_unit->getGuid());
		}

		sApplication->clearMouseUp();
	}

	// Right clicking a portrait
	else if (m_unit != nullptr)
	{
		if (sApplication->mouseUp(sf::Mouse::Right) && isMousedOver() && dynamic_cast<World*>(getOwner()) != nullptr)
		{
			sApplication->clearMouseUp();
			openContextMenu();
		}
	}

	if (auto confirmBox = sApplication->popConfirmBox({ ConfirmMessageBox::ConfirmCode_ResetDungeons }))
	{
		if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
		{
			// ResetDungeons opcode UNRESOLVED (op38 turned out to be RequestRespawn). Skip until captured.
				if (!GamePacket::validOpcode(Opcode::Client_ResetDungeons))
					return;
				GP_Client_ResetDungeons pk;
			sConnector->sendPacket(pk.build(StlBuffer{}));
		}
	}
}

void UnitFrame::render()
{
	__super::render();

	auto world = dynamic_cast<World*>(getOwner());

	if (m_unit != nullptr)
	{
		// Dispel logic needs to match target's friendlyness status
		if (world != nullptr && world->myself() != nullptr)
		{
			bool thisFriendly = world->myself()->seesAsFriendly(*m_unit);

			if (m_lastFriendlieness != thisFriendly)
				refreshDispelTypeLogic();

			m_lastFriendlieness = thisFriendly;
		}

		if (m_hpPctRender > m_unit->getHealthPct())
		{
			m_hpPctRender -= 2.f * sApplication->delta();

			if (m_hpPctRender < m_unit->getHealthPct())
				m_hpPctRender = m_unit->getHealthPct();
		}

		if (m_hpPctRender < m_unit->getHealthPct())
		{
			m_hpPctRender += 2.f * sApplication->delta();

			if (m_hpPctRender > m_unit->getHealthPct())
				m_hpPctRender = m_unit->getHealthPct();
		}

		if (m_manaPctRender > m_unit->getManaPct())
		{
			m_manaPctRender -= 2.f * sApplication->delta();

			if (m_manaPctRender < m_unit->getManaPct())
				m_manaPctRender = m_unit->getManaPct();
		}

		if (m_manaPctRender < m_unit->getManaPct())
		{
			m_manaPctRender += 2.f * sApplication->delta();

			if (m_manaPctRender > m_unit->getManaPct())
				m_manaPctRender = m_unit->getManaPct();
		}
	}

	decidePortrait();

	// For clicking on the frame
	m_bottomRightCorner = m_topLeftCorner + sf::Vector2i(sf::Vector2f(m_frameSprite->getGlobalBounds().width, m_frameSprite->getGlobalBounds().height));
	
	m_frameSprite->render(sf::Vector2f(m_topLeftCorner));

	if (m_portrait != nullptr)
		m_portrait->renderAsCircle(sf::Vector2f(m_portraitPos + m_topLeftCorner), m_portraitRadius, { 0, m_portraitOffset });

	renderBarCrop(*m_hpSprite, m_hpTopLeft + m_topLeftCorner, m_hpPctRender);
	renderBarCrop(*m_manaSprite, m_manaTopLeft + m_topLeftCorner, m_manaPctRender);

	// Elite
	if (m_frameStyle == UnitFrame::MyTarget && m_unit != nullptr && m_unit->getVariable(ObjDefines::Variable::Elite) != 0)
		m_eliteSprite->render(sf::Vector2f(m_elitePos + m_topLeftCorner));

	// Boss
	if (m_frameStyle == UnitFrame::MyTarget && m_unit != nullptr && m_unit->getVariable(ObjDefines::Variable::Boss) != 0)
		m_bossSprite->render(sf::Vector2f(m_bossPos + m_topLeftCorner));

	m_levelFrameSprite->setHotspotEasy(true, true);
	m_levelFrameSprite->render(sf::Vector2f(m_topLeftCorner + m_lvlTxtPos));

	int level = m_unit != nullptr ? m_unit->getLevel() : 0;
	m_lvlTxt->setData(m_topLeftCorner.x + m_lvlTxtPos.x - 24, m_topLeftCorner.y + m_lvlTxtPos.y, to_string(level), 50, TextBox::AlignCenterBounds, true, 1.f, sf::Color(0, 0, 0, 127));
	m_lvlTxt->draw();

	if (isMousedOver() && m_unit != nullptr)
	{
		m_hpTxt->setOriginalString(Util::fmtStr("%d / %d", m_unit->getHealth(), m_unit->getMaxHealth()));
		m_hpTxt->draw(m_topLeftCorner.x + m_hpTxtPos.x - int(m_hpTxt->getGlobalBounds().width / 2.f), m_topLeftCorner.y + m_hpTxtPos.y);
		
		m_manaTxt->setOriginalString(Util::fmtStr("%d / %d", m_unit->getMana(), m_unit->getMaxMana()));
		m_manaTxt->draw(m_topLeftCorner.x + m_mpTxtPos.x - int(m_manaTxt->getGlobalBounds().width / 2.f), m_topLeftCorner.y + m_mpTxtPos.y);
	}
	
	m_hpPctTxt->setOriginalString(to_string((int)ceil(m_hpPctRender * 100.f)) + "%");
	m_hpPctTxt->draw(m_topLeftCorner.x + m_hpPctPos.x, m_topLeftCorner.y + m_hpPctPos.y);

	m_manaPctTxt->setOriginalString(to_string((int)ceil(m_manaPctRender * 100.f)) + "%");
	m_manaPctTxt->draw(m_topLeftCorner.x + m_mpPctPos.x, m_topLeftCorner.y + m_mpPctPos.y);

	if (m_frameStyle != FrameStyle::MyTarget && m_unit != nullptr && world != nullptr && world->isPartyLeader(m_unit->getGuid()))
		m_partyLeaderSprite->render(sf::Vector2f(m_topLeftCorner + m_leaderPos));

	if (m_castBar != nullptr)
	{
		m_castBar->getTopLeftCornerRef() = m_topLeftCorner;
		m_castBar->getTopLeftCornerRef().y += 200;
		m_castBar->getTopLeftCornerRef().x += 50;
		m_castBar->attemptRender();
	}

	if (m_msgTimer > 0.f)
	{
		float alpha = 1.f;

		if (m_msgTimer > 1.5f)
			alpha = (2.f - m_msgTimer) / 0.5f;
		else if (m_msgTimer < 0.5f)
			alpha = m_msgTimer / 0.5f;

		// Fade in and out
		sf::Color color = m_combatMsgTxt->getOriginalColor();
		color.a = static_cast<uint8_t>(255.f * alpha);
		m_combatMsgTxt->setFillColor(color);
		m_combatMsgTxt->draw(m_topLeftCorner.x + m_combatMsgPos.x, m_topLeftCorner.y + m_combatMsgPos.y);

		m_msgTimer -= sApplication->delta() * 2.f;
	}

	if (m_unit != nullptr && m_unit->getVariable(ObjDefines::Variable::InCombat))
	{
		m_combatSprite->setHotspotEasy(true, true);
		m_combatSprite->render(sf::Vector2f(m_topLeftCorner + m_combatPos));
	}

	// In arena queue render
	if (m_unit != nullptr && m_unit->isLocal() && m_unit->getVariable(ObjDefines::Variable::InArenaQueue) != 0)
	{
		if (!m_inArenaQueue || m_criticalStatusSprite == nullptr)
		{
			m_criticalStatusSprite = sContentMgr->spawnSprite("arena_queue_icon.png");
			m_criticalStatusSprite->setHotspotEasy(true, true);
		}

		m_hasBrokenIcon = false;
		m_inArenaQueue = true;
		m_criticalStatusPsi->moveTo(sf::Vector2f(m_topLeftCorner + m_repairPos) + sf::Vector2f(2.f, 7.f), true);
		m_criticalStatusPsi->update(sApplication->getElapsed());
		sApplication->canvas().draw(*m_criticalStatusPsi);
		m_criticalStatusSprite->render(sf::Vector2f(m_topLeftCorner + m_repairPos));
	}

	// Broken equipment
	else if (m_unit != nullptr && m_unit->isLocal() && dynamic_pointer_cast<ClientPlayer>(m_unit)->hasBrokenEquipment())
	{
		if (!m_hasBrokenIcon || m_criticalStatusSprite == nullptr)
		{
			sContentMgr->playSound(SfxId::AlertSoulstoneEmpty);
			m_criticalStatusSprite = sContentMgr->spawnSprite("broken_item_uframe_icon.png");
			m_criticalStatusSprite->setHotspotEasy(true, true);
		}

		m_inArenaQueue = false;
		m_hasBrokenIcon = true;
		m_criticalStatusPsi->moveTo(sf::Vector2f(m_topLeftCorner + m_repairPos) + sf::Vector2f(2.f, 7.f), true);
		m_criticalStatusPsi->update(sApplication->getElapsed());
		sApplication->canvas().draw(*m_criticalStatusPsi);
		m_criticalStatusSprite->render(sf::Vector2f(m_topLeftCorner + m_repairPos));
	}
	else
	{
		m_criticalStatusSprite = nullptr;
		m_inArenaQueue = false;
		m_hasBrokenIcon = false;
	}

	if (m_frameStyle != FrameStyle::LocalPlayer)
	{
		string nameupper = m_unit != nullptr ? m_unit->getName() : "Unknown";
		transform(nameupper.begin(), nameupper.end(), nameupper.begin(), ::toupper);

		m_nameTxt->setOriginalColor(m_unit != nullptr ? m_unit->getNameColor() : sf::Color(125, 125, 125));
		m_nameTxt->setOriginalString(nameupper);
		m_nameTxt->draw(int(m_topLeftCorner.x + m_namePos.x - m_nameTxt->getGlobalBounds().width), m_topLeftCorner.y + m_namePos.y);
	}

	if (isMousedOver() && !sApplication->isTooltipRegistered() && m_unit != nullptr)
		m_unit->useTooltip();
}

sf::IntRect UnitFrame::renderBarCrop(Sprite& spr, const sf::Vector2i topLeftCorner, float pct)
{
	if (pct <= 0)
		return sf::IntRect{};

	if (pct > 1.f)
		pct = 1.f;

	sf::IntRect r;
	r.top = 0;
	r.left = 0;
	r.width = static_cast<int>(ceil(float(spr.getTexture()->getSize().x) * pct));
	r.height = spr.getTexture()->getSize().y;

	spr.setTextureRect(r);
	spr.render(sf::Vector2f(topLeftCorner));
	return r;
}

void UnitFrame::decidePortrait()
{
	if (auto npc = dynamic_pointer_cast<ClientNpc>(m_unit))
	{
		if (npc->getVariable(ObjDefines::Variable::DynGreyTagged))
		{
			setPortraitByName("grey");
		}
		else
		{
			string defaultModel = sContentMgr->db("npc_models").data(npc->getVariable(ObjDefines::Variable::ModelId), "name");
			string customPortrait = sContentMgr->db("npc_template").data(npc->getEntry(), "portrait");
			const string& portName = customPortrait.empty() ? defaultModel : customPortrait;

			if (!setPortraitByName(portName))
			{
				m_invalidPortraits.insert(portName);

				switch (npc->faction())
				{
					case UnitDefines::Faction::Friendly: setPortraitByName("friendly"); break;
					case UnitDefines::Faction::Neutral: setPortraitByName("neutral"); break;
					default: setPortraitByName("hostile"); break;
				}
			}
		}
	}
	else if (auto player = dynamic_pointer_cast<ClientPlayer>(m_unit))
	{
		const string genderStr = player->getGender() == PlayerDefines::Gender::Male ? "male" : "female";
		setPortraitByName(Util::fmtStr("%s (%d)", genderStr.c_str(), player->getPortrait()));
	}
	else
	{
		setPortraitByName("grey");
	}
}

// This is a function so that if we want to change the process for rendering a bar, we don't have to edit the same code twice over for both HP/MP.
void UnitFrame::renderBar(Sprite& spr, const sf::Vector2i topLeftCorner, const float maxWidth, const float pct)
{
	spr.renderStretch(sf::Vector2f(topLeftCorner), 
		sf::Vector2f(static_cast<float>(topLeftCorner.x) + ceil(maxWidth * min(1.0f, pct)), static_cast<float>(topLeftCorner.y) + spr.getGlobalBounds().height));
}

void UnitFrame::setFrameStyle(const FrameStyle v)
{
	if (m_frameSprite != nullptr && m_frameStyle == v)
		return;

	m_frameStyle = v;

	BuffDebuffRenderer::setDirection(m_frameStyle == FrameStyle::MyTarget ? BuffDebuffRenderer::Direction::RightLeft : BuffDebuffRenderer::Direction::LeftRight);

	switch (m_frameStyle)
	{
		case FrameStyle::MyTarget:
		{
			m_hpTopLeft = { 3, 39 };
			m_manaTopLeft = { 23, 70 };
			m_lvlTxtPos = { 330, 112 };
			m_portraitPos = { 329, 73 };
			m_hpPctPos = { 250, 46 };
			m_mpPctPos = { 250, 74 };
			m_namePos = { 300, 10 };
			m_elitePos = { 255, -2 };
			m_bossPos = { 255, -2 };
			m_aurasOffset = { 265, 108 };
			m_combatPos = { 332, 115 };
			m_combatMsgPos = { 328, 73 };
			m_hpTxtPos = { 165, 45 };
			m_mpTxtPos = { 165, m_hpTxtPos.y + 28 };
			m_frameSprite = sContentMgr->spawnSprite("unit_frame_reverse.png");
			m_hpSprite = sContentMgr->spawnSprite("unit_frame_hp_reverse.png");
			m_manaSprite = sContentMgr->spawnSprite("unit_frame_mp_reverse.png");
			m_castBar = make_shared<CastBar>(*this, 0);
			m_portraitRadius = 39;
			m_auraSpacing.x = 33;
			m_auraSpacing.y = 33;
			m_auraButtonFrame = "gameicon30";
			break;
		}
		case FrameStyle::LocalPlayer:
		{
			m_leaderPos = { 16, 40 };
			m_hpTopLeft = { 74, 39 };
			m_manaTopLeft = { 84, 70 };
			m_lvlTxtPos = { 71, 105 };
			m_portraitPos = { 44, 73 };
			m_hpPctPos = { 91, 46 };
			m_mpPctPos = { 91, 74 };
			m_aurasOffset = { 95, 108 };
			m_combatPos = { 73, 108 };
			m_combatMsgPos = { 44, 73 };
			m_repairPos = { 15, 105 };
			m_hpTxtPos = { 215, 45 };
			m_mpTxtPos = { 215, m_hpTxtPos.y + 28 };
			m_frameSprite = sContentMgr->spawnSprite("unit_frame.png");
			m_hpSprite = sContentMgr->spawnSprite("unit_frame_hp.png");
			m_manaSprite = sContentMgr->spawnSprite("unit_frame_mp.png");
			m_castBar = nullptr;
			m_portraitRadius = 39;
			m_auraSpacing.x = 33;
			m_auraSpacing.y = 33;
			m_auraButtonFrame = "gameicon30";
			break;
		}
		case FrameStyle::PartyMember:
		{
			m_leaderPos = { 10, 14 };
			m_hpTopLeft = { 45, 11 };
			m_manaTopLeft = { 55, 33 };
			m_lvlTxtPos = { 51, 54 };
			m_portraitPos = { 30, 36 };
			m_hpPctPos = { 66, 13 };
			m_mpPctPos = { 66, 33 };
			m_namePos = { 200, -13 };
			m_aurasOffset = { 76, 58 };
			m_combatPos = { 53, 57 };
			m_combatMsgPos = { 44, 73 };
			m_repairPos = { 15, 105 };
			m_hpTxtPos = { 165, 13 };
			m_mpTxtPos = { 165, m_hpTxtPos.y + 20 };
			m_frameSprite = sContentMgr->spawnSprite("unit_frame_party.png");
			m_hpSprite = sContentMgr->spawnSprite("unit_frame_hp_party.png");
			m_manaSprite = sContentMgr->spawnSprite("unit_frame_mp_party.png");
			m_castBar = nullptr;
			m_portraitRadius = 26;
			m_auraSpacing.x = 22;
			m_auraSpacing.y = 22;
			m_auraButtonFrame = "gameicon19";
			break;
		}
	}

	registerAuraObjs(&getTopLeftCornerRef());
	ASSERT(m_frameSprite != nullptr);
}
		
void UnitFrame::playMessage(const string& msg, sf::Color color)
{
	color.a = 255;
	m_combatMsgTxt->setScale(1.f, 1.f);
	m_combatMsgTxt->setString(msg);
	m_combatMsgTxt->setOriginalColor(color);
	m_msgTimer = 2.f;
		
	if (m_combatMsgTxt->getGlobalBounds().width >= 80.f)
	{
		float downscale = 80.f / m_combatMsgTxt->getGlobalBounds().width;
		m_combatMsgTxt->setScale(downscale, downscale);
	}

	sf::FloatRect textRect = m_combatMsgTxt->getLocalBounds();
	m_combatMsgTxt->setOrigin(textRect.left + (textRect.width/2.0f), textRect.top + (textRect.height/2.0f));
}

void UnitFrame::setUnit(shared_ptr<ClientUnit> unit)
{
	m_unit = unit;
	m_msgTimer = 0;

	BuffDebuffRenderer::setUnit(unit);

	if (m_castBar != nullptr)
		m_castBar->setUnit(unit);

	if (m_unit == nullptr && !m_offline)
	{
		setHidden(true);
		return;
	}

	if (!m_offline)
	{
		if (auto world = dynamic_cast<World*>(getOwner()))
		{
			if (world->myself() != nullptr)
				m_lastFriendlieness = world->myself()->seesAsFriendly(*m_unit);
		}
	
		setBuffs(m_unit->getBuffs());
		setDebuffs(m_unit->getDebuffs());

		m_hpPctRender = unit->getHealthPct();
		m_manaPctRender = unit->getManaPct();
	}

	setHidden(false);
	decidePortrait();
}

bool UnitFrame::setPortraitByName(string name)
{
	if (m_unit == nullptr)
		name = "grey";

	string textureName = "portrait_" + name + ".png";

	// Redundant
	if (m_portrait != nullptr && m_portrait->getTextureName() == textureName)
		return true;

	if (m_invalidPortraits.find(name) != m_invalidPortraits.end())
		return false;

	m_portrait = sContentMgr->spawnPortrait(textureName);
	
	if (m_portrait == nullptr)
		return false;

	m_portraitOffset = sContentMgr->getPortraitOffset(*m_portrait);
	return true;
}

/*static*/
string UnitFrame::ctxMenuOptionStr(const CtxMenuOptions id)
{
	switch (id)
	{
		case CtxMenuOptions::DdoWhisper: return "Whisper";
		case CtxMenuOptions::DdoInspect: return "Inspect";
		case CtxMenuOptions::DdoInvite: return "Invite";
		case CtxMenuOptions::DdoTrade: return "Trade";
		case CtxMenuOptions::DdoDuel: return "Duel";
		case CtxMenuOptions::DdoCancel: return "Cancel";
		case CtxMenuOptions::DdoKick: return "Kick";
		case CtxMenuOptions::DdoLeave: return "Leave Party";
		case CtxMenuOptions::DdoResetDungeons: return "Reset Dungeons";
		case CtxMenuOptions::DdoPromote: return "Promote";
		case CtxMenuOptions::DdoLeaveArenaQueue: return "Leave Queue";
	}

	return "";
}

// Build + register the right-click context menu for this unit. Public so the Lua unit frames (which replace
// the C++ ones, that are force-hidden but still alive) can open the SAME menu via a thin command — the result
// routes back through notifyCtxMenuClicked regardless of this frame being hidden.
void UnitFrame::openContextMenu(RenderObjectHolder* host, const std::vector<std::string>& extraLines)
{
	auto owner = dynamic_cast<World*>(getOwner());
	if (owner == nullptr || m_unit == nullptr)
		return;
	if (host == nullptr)
		host = owner;   // default: the C++ right-click path registers on World

	vector<string> ctxLines;

	// A party member other than ourselves
	if (m_frameStyle == FrameStyle::PartyMember)
	{
		if (owner->isPartyLeader(owner->getMyselfGuid()))
		{
			ctxLines = {
					ctxMenuOptionStr(CtxMenuOptions::DdoWhisper),
					ctxMenuOptionStr(CtxMenuOptions::DdoTrade),
					ctxMenuOptionStr(CtxMenuOptions::DdoPromote),
					ctxMenuOptionStr(CtxMenuOptions::DdoKick),
			};
		}
		else
		{
			ctxLines = {
					ctxMenuOptionStr(CtxMenuOptions::DdoWhisper),
			};
		}
	}

	// Target
	else if (!m_unit->isLocal())
	{
		if (m_unit->getType() == ClientUnit::Type::Player)
		{
			ctxLines = {
					ctxMenuOptionStr(CtxMenuOptions::DdoWhisper),
					ctxMenuOptionStr(CtxMenuOptions::DdoInvite),
					ctxMenuOptionStr(CtxMenuOptions::DdoTrade),
					ctxMenuOptionStr(CtxMenuOptions::DdoDuel),
			};
		}
		else
		{
			// Npc
		}
	}

	// Myself in a party
	else if (owner->isPartyMember(owner->getMyselfGuid()))
	{
		ctxLines = {
				ctxMenuOptionStr(CtxMenuOptions::DdoLeave),
				ctxMenuOptionStr(CtxMenuOptions::DdoResetDungeons),
		};
	}

	// Myself
	else if (m_unit->isLocal())
	{
		ctxLines = {
				ctxMenuOptionStr(CtxMenuOptions::DdoResetDungeons),
		};
	}

	if (m_unit->isLocal() && m_unit->getVariable(ObjDefines::Variable::InArenaQueue) != 0)
		ctxLines.push_back(ctxMenuOptionStr(CtxMenuOptions::DdoLeaveArenaQueue));

	// Lua-supplied extras (e.g. Lock/Unlock), handled on the Lua side via the menu-result event.
	for (const auto& line : extraLines)
		ctxLines.push_back(line);

	if (!ctxLines.empty())
	{
		sContentMgr->playSound(SfxId::ButtonClick);
		ctxLines.push_back(ctxMenuOptionStr(CtxMenuOptions::DdoCancel));
		host->registerContextMenu(World::Interface::CtxMenuCurrent, getId(), sApplication->mousePos(), ctxLines);
	}
}

void UnitFrame::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
{
	if (id != World::Interface::CtxMenuCurrent)
		return;

	if (m_unit == nullptr)
		return;

	if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoWhisper))
	{
		if (auto owner = dynamic_cast<World*>(getOwner()))
		{
			if (auto gameChat = dynamic_pointer_cast<GameChat>(owner->getRenderObject(World::Interface::GameChatBox)))
				gameChat->setWhispering(m_unit->getName());
		}
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoTrade))
	{
		GP_Client_OpenTradeWith pk;
		pk.m_playerName = m_unit->getName();
		sConnector->sendPacket(pk.build(StlBuffer{}));
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoInvite))
	{
		GP_Client_PartyInviteMember pk;
		pk.m_playerName = m_unit->getName();
		sConnector->sendPacket(pk.build(StlBuffer{}));
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoDuel))
	{
		GP_Client_CastSpell packet;
		packet.m_targetGuid = m_unit->getGuid();
		packet.m_spellId = SpellDefines::StaticSpells::OfferDuel;
		sConnector->sendPacket(packet.build(StlBuffer{}));
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoLeave))
	{
		GP_Client_PartyChanges pk;
		pk.m_leaveParty = true;
		sConnector->sendPacket(pk.build(StlBuffer{}));
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoKick))
	{
		GP_Client_PartyChanges pk;
		pk.m_kickPlayerGuid = m_unit->getGuid();
		sConnector->sendPacket(pk.build(StlBuffer{}));
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoPromote))
	{
		GP_Client_PartyChanges pk;
		pk.m_promotePlayerGuid = m_unit->getGuid();
		sConnector->sendPacket(pk.build(StlBuffer{}));
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoResetDungeons))
	{
		sApplication->spawnPopup("Reset your dungeons?", ConfirmMessageBox::ConfirmBox_Yes, ConfirmMessageBox::ConfirmCode_ResetDungeons);
	}
	else if (lineClicked == ctxMenuOptionStr(CtxMenuOptions::DdoLeaveArenaQueue))
	{
		GP_Client_UpdateArenaStatus pk;
		pk.m_leaveQueue = true;
		sConnector->sendPacket(pk.build(StlBuffer{}));
	}
}
