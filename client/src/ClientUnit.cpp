#include "stdafx.h"
#include "ClientUnit.h"
#include "ContentMgr.h"
#include "ModelScriptRender.h"
#include "ClientObjectOverhead.h"
#include "World.h"
#include "Sprite.h"
#include "ClientNpc.h"
#include "Application.h"
#include "GameChat.h"
#include "ClientMap.h"
#include "Connector.h"
#include "WorldSpellAnimation.h"
#include "ClientPlayer.h"
#include "Minimap.h"
#include "Options.h"
#include "Tooltip.h"
#include "TextBox.h"

#include "..\Rand.h"
#include "..\Math.h"
#include "..\SqlConnector\QueryResult.h"
#include "..\Shared\GamePacketClient.h"
#include "..\StringHelpers.h"
#include "..\Shared\Config.h"
#include "..\Shared\NpcDefines.h"

#include <assert.h>


ClientUnit::ClientUnit(const int guid, ClientMap* clientMap) :
	ClientObject(guid, clientMap),
	MutualUnit(getWorldPositionRef()),
	m_animationId(UnitDefines::Stance),
	m_isLocal(false),
	m_nextFootstep(0),
	m_previousAlive(false),
	m_castSpellId(0),
	m_castStartTime(0),
	m_castStopTime(0),
	m_brightenPct(0)
{
	m_overheads = make_unique<ClientObjectOverhead>(*this);
	ASSERT(m_selectedSprite = sContentMgr->spawnSprite("unit_selected.png"));
	m_selectedSprite->setHotspotEasy(true, true);

	setVariable(ObjDefines::Variable::Health, 1);
	setVariable(ObjDefines::Variable::MaxHealth, 1);
}

ClientUnit::~ClientUnit()
{

}

/*virtual*/
void ClientUnit::update() /*override*/
{
	__super::update();

	if (m_previousAlive != isAlive() && !isAlive())
		died();

	doFootstepSounds();
	m_previousAlive = isAlive();

	auto desiredAnimation = UnitDefines::Stance;
	
	pumpSpline(m_serverSpline, m_serverLocation, sApplication->delta());

	if (!__super::pumpSpline() && __super::hasSpline() && !isSlidingSpline())
		desiredAnimation = UnitDefines::Run;

	if (!isAlive())
		desiredAnimation = UnitDefines::Die;

	if (!isAnimationPlayOnce(m_animationId) || isAnimationFinished() || (!isAnimationPlayableWhileMoving(m_animationId) && __super::hasSpline()))
		setAnimation(desiredAnimation);

	// Can't return true unless a valid pointer to world is available
	if (popClickedOnInWorld(sf::Mouse::Left))
		getWorld()->setSelectedGuid(getGuid());

	// Toggle
	if (popClickedOnInWorld(sf::Mouse::Right))
	{
		// Loot
		if (getVariable(ObjDefines::DynLootable))
		{
			GP_Client_CastSpell packet;
			packet.m_targetGuid = getGuid();
			packet.m_spellId = SpellDefines::LootUnit;
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}

		// Gossip
		else if (getVariable(ObjDefines::DynGossipStatus) != ObjDefines::GossipNone)
		{
			GP_Client_CastSpell packet;
			packet.m_targetGuid = getGuid();
			packet.m_spellId = SpellDefines::NpcGossip;
			sConnector->sendPacket(packet.build(StlBuffer{}));
		}
	}

	m_brightenPct -= sApplication->delta() * 5.f;

	if (mousedInWorld())
	{
		useTooltip();

		if (getVariable(ObjDefines::DynLootable))
			sApplication->queueCursor("cursor_loot.png");
		else if (getVariable(ObjDefines::DynGossipStatus) != ObjDefines::GossipNone)
			sApplication->queueCursor("cursor_gossip.png");
		else if (isAlive() && getWorld()->myself() != nullptr && !getWorld()->myself()->seesAsFriendly(*this))
			sApplication->queueCursor("cursor_sword.png");

		// Brighten when moused over
		m_brightenPct = max(0.1f, m_brightenPct);
	}
}

/*virtual*/
void ClientUnit::render() /*override*/
{
	__super::render();

	drawAuras_below();

	m_topLeftCorner = getScreenPosition();
	m_bottomRightCorner = getScreenPosition();
	
	m_modelAlpha = uint8_t(255.f * getAlphaPct() * getFadeInPct());
	
	if (m_npcModel != nullptr)
	{
		setModelScriptRenderVariables(*m_npcModel);
		processBounds(m_npcModel->render(getScreenPosition(), m_animationId, computeDirection(getOrientation()), false));
	}
	else
	{
		setHeight(50);

		for (auto& itr : m_bodyParts)
		{
			if (itr.first == BodyPart::WeaponRanged && m_sheath != itr.first)
				continue;

			if (itr.first == BodyPart::Weapon && m_sheath != itr.first)
				continue;

			setModelScriptRenderVariables(*itr.second);
			processBounds(itr.second->render(getScreenPosition(), m_animationId, computeDirection(getOrientation()), false));
		}
	}

	m_modelColor = sf::Color::Transparent;
	
	drawAuras_top();
	
	if (getMap() != nullptr)
	{
		// Name rendering uses a cached texture, and so when the option for "Show ranks" changes, we may need to dedraw name string texture
		if (m_showRanks != static_cast<bool>(sConfig->getCache(Options::System_PlayerRanksTick)))
		{
			m_overheads->refreshName();
			m_showRanks = sConfig->getCache(Options::System_PlayerRanksTick);
		}

		m_overheads->setAlpha(m_modelAlpha);
		getMap()->registerObjectOverhead(m_overheads);

		// Update some info to the world gui
		if (getWorld() != nullptr)
		{
			// Minimap
			if (auto minimap = dynamic_pointer_cast<Minimap>(getWorld()->getRenderObject(World::Interface::MinimapObj)))
			{
				Minimap::DotType dot = Minimap::DotType::FriendlyUnit;

				switch (faction())
				{
					case UnitDefines::Faction::PlayerDefault: dot = Minimap::DotType::FriendlyUnit; break;
					case UnitDefines::Faction::Friendly: dot = Minimap::DotType::FriendlyUnit; break;
					case UnitDefines::Faction::Neutral: dot = Minimap::DotType::NeutralUnit; break;
					case UnitDefines::Faction::Hostile: dot = Minimap::DotType::EnemyUnit; break;
					case UnitDefines::Faction::PvP: dot = Minimap::DotType::EnemyUnit; break;
				}

				switch (getVariable(ObjDefines::Variable::DynGossipStatus))
				{
					case ObjDefines::GossipStatus::QuestAvailable: dot = Minimap::DotType::QuestStart; break;
					case ObjDefines::GossipStatus::QuestComplete: dot = Minimap::DotType::QuestComplete; break;
					case ObjDefines::GossipStatus::GossipAvailable: dot = Minimap::DotType::GossipAvailable; break;
				}

				if (!isAlive())
					dot = Minimap::DotType::DeadUnit;

				minimap->registerDot(getGuid(), getWorldPosition(), dot);
			}

			// Selected sphere
			if (getWorld()->getSelectedGuid() == getGuid())
			{
				m_selectedSprite->setColor(getNameColor());
				getMap()->registerExtraRender(m_selectedSprite, GameMap::Defines::Layer2, computeRawScreenPositioni());
			}
		}
	}
	else
	{
		m_overheads->draw();
	}
		
	m_animFreezeFrame = 0;
	m_animPlaybackSpeed = 0.f;
}
		
bool ClientUnit::isMousedOver(const bool useRealPosition /*= false*/)
{
	if (m_overheads->isMousedOver(useRealPosition))
		return true;

	return __super::isMousedOver(useRealPosition);
}

void ClientUnit::expireTooltip()
{
	m_tooltip = nullptr;
}

void ClientUnit::useTooltip()
{
	if (getWorld() == nullptr)
		return;

	if (::clock() > m_nextDrawTooltip)
	{
		expireTooltip();
		m_nextDrawTooltip = ::clock() + 3000;
	}

	if (m_tooltip == nullptr)
	{
		m_tooltip = make_shared<Tooltip>(*getWorld(), sf::Vector2i{});
		m_tooltip->setShadowOffset(1);
		m_tooltip->addLine("arial.ttf", 15, getName(), getNameColor());

		if (!getSubName().empty())
			m_tooltip->addLine("arial.ttf", 15, getSubName());

		string typeName;

		if (auto player = dynamic_cast<ClientPlayer*>(this))
			typeName = PlayerFunctions::className(player->getClass());
		else if (auto npc = dynamic_cast<ClientNpc*>(this))
			typeName = NpcFunctions::typeName(static_cast<NpcDefines::Type>(atoi(sContentMgr->db("npc_template").data(npc->getEntry(), "type").c_str())));
		
		string description = Util::fmtStr("Level %d %s", getLevel(), typeName.c_str());
		m_tooltip->addLine("arial.ttf", 15, description);
	}

	m_tooltip->moveTo(sf::Vector2i(sApplication->centerOfScreen().x + 400, sApplication->sH() - 180 - m_tooltip->getHeight()));
	sApplication->setTooltip(m_tooltip);
}

void ClientUnit::flashBrighten(const float pct /*= 0.2f*/)
{
	m_brightenPct = pct;
}

void ClientUnit::setModelScriptRenderVariables(ModelScriptRender& r) const
{
	sf::Color color = r.getColor();
	color.a = m_modelAlpha;

	r.setColor(color);
	r.setBrightnessPct(m_brightenPct);
	r.setColorOverlay(m_modelColor != sf::Color::Transparent ? m_modelColor : r.defaultOverlayColor());
	
	if (m_animationId == UnitDefines::AnimId::Run)
		r.setSpeed(getSpeed());
	else
		r.setSpeed(1.f);
	
	r.setTimerRatio(m_animPlaybackSpeed);
	r.setFreezeFrame(m_animFreezeFrame);
	r.setScale(m_scale);
}

void ClientUnit::processBounds(sf::FloatRect& spr)
{
	if (spr.width == 0)
	{
		m_topLeftCorner = m_bottomRightCorner = sf::Vector2i{};
		return;
	}

	m_topLeftCorner.x = min(m_topLeftCorner.x, static_cast<int>(spr.left));
	m_topLeftCorner.y = min(m_topLeftCorner.y, static_cast<int>(spr.top));
	m_bottomRightCorner.x = max(m_bottomRightCorner.x, static_cast<int>(spr.left) + static_cast<int>(spr.width));
	m_bottomRightCorner.y = max(m_bottomRightCorner.y, static_cast<int>(spr.top) + static_cast<int>(spr.height));
}

void ClientUnit::notifySubnameChanged() /*final*/
{
	m_overheads->refreshName();
}

void ClientUnit::notifyNameChanged() /*final*/
{
	m_overheads->refreshName();
}

void ClientUnit::notifyVariableChange(const ObjDefines::Variable var, const int value) /*override*/
{
	__super::notifyVariableChange(var, value);

	switch (var)
	{
		case ObjDefines::Variable::DynLootable:
		{
			if (value && !hasAura(SpellDefines::LootVisual))
				auraAnimationIncrement(SpellDefines::LootVisual);
			else if (!value)
				auraAnimationDecrement(SpellDefines::LootVisual);

			break;	
		}
		case ObjDefines::Variable::DynGossipStatus:
		{
			// QuestComplete
			if (value == ObjDefines::GossipStatus::QuestComplete && !hasAura(SpellDefines::QuestDone))
				auraAnimationIncrement(SpellDefines::QuestDone);
			else if (value != ObjDefines::GossipStatus::QuestComplete)
				auraAnimationDecrement(SpellDefines::QuestDone);

			// QuestAvailable
			if (value == ObjDefines::GossipStatus::QuestAvailable && !hasAura(SpellDefines::QuestDone))
				auraAnimationIncrement(SpellDefines::QuestReady);
			else if (value != ObjDefines::GossipStatus::QuestAvailable)
				auraAnimationDecrement(SpellDefines::QuestReady);

			break;	
		}
		case ObjDefines::Variable::ModelId:
		{
			setNpcModel(sContentMgr->db("npc_models").data(value, "name"));

			int val = atoi(sContentMgr->db("npc_models").data(value, "height").c_str());

			if (m_scale != 1.f)
				val = int((float)val * m_scale);

			setHeight(val);
			break;
		}
		case ObjDefines::Variable::MechanicMask:
		{
			if (Util::maskHas(value, SpellDefines::Mechanics::Stun) || Util::maskHas(value, SpellDefines::Mechanics::Sleep) || Util::maskHas(value, SpellDefines::Mechanics::Incapacitated))
			{
				if (!isSlidingSpline())
					clearSpline();
			}

			break;
		}
		case ObjDefines::Variable::ModelScale:
		{
			if (value != 0)
			{
				float fvalue = float(value) / 100.f;
				m_scale = fvalue;
				setHeight(int((float)mouseableHeight() * m_scale));
			}

			break;
		}
		case ObjDefines::Variable::IsAnimating:
		{
			if (value != 0)
			{
				if (!isSlidingSpline())
					clearSpline();
			}

			break;
		}
		case ObjDefines::Variable::MoveSpeedPct:
		{
			// MoveSpeedPct is a percent (100 == normal); getSpeed() is consumed as a
			// 1.0-based multiplier (animation rate = duration/getSpeed(), spline speed).
			setSpeed(value / 100.f);
			break;
		}
		case ObjDefines::Variable::Health:
		{
			if (value == 0)
			{
				if (auto world = getWorld())
				{
					if (getGuid() == world->getSelectedGuid())
						world->setSelectedGuid(0);
				}

				setAnimation(UnitDefines::AnimId::Die);
				playDeathSound();
			}

			break;
		}
	}
}

void ClientUnit::setNpcModel(const string& modelName)
{
	if (auto ptr = sContentMgr->spawnModelScript(modelName))
	{
		m_npcModel = make_unique<ModelScriptRender>();
		m_npcModel->setModelScript(move(ptr));
	}
	else
	{
		m_npcModel = nullptr;
	}
}

void ClientUnit::doFootstepSounds()
{
	if (m_footstepSounds.empty())
		return;

	if (Util::maskHas(getVariable(ObjDefines::Variable::MechanicMask), SpellDefines::Mechanics::Stealth))
		return;

	if (std::clock() < m_nextFootstep)
		return;

	ModelScript* script = nullptr;
	
	if (m_npcModel != nullptr)
		script = m_npcModel->getScript();
	else if (!m_bodyParts.empty())
		script = m_bodyParts.begin()->second->getScript();

	if (script == nullptr)
		return;

	if (auto runAnim = script->getAnim(UnitDefines::AnimId::Run))
	{
		float animationTimer = runAnim->durationInSeconds() / getSpeed();
		clock_t timeBetweenSteps = static_cast<clock_t>(animationTimer / 2.f * 1000.f);

		if (timeBetweenSteps == 0)
			return;

		m_nextFootstep = std::clock() + timeBetweenSteps;

		// This check is at the very end because we want the m_nextFootstep logic to ensure there's a delay between the first tick moving and the first sound
		if (getAnimation() == UnitDefines::AnimId::Run)
		{
			int soundIdx = Util::irand(0, m_footstepSounds.size() - 1);
			playSound(m_footstepSounds[soundIdx], 5.f, 0.5f);
		}
	}
}

void ClientUnit::setAnimation(const UnitDefines::AnimId id)
{
	if (m_animationId == id)
		return;

	m_animationId = id;

	if (m_npcModel != nullptr)
		m_npcModel->reset();

	for (auto& itr : m_bodyParts)
		itr.second->reset();

	if (m_animationId == UnitDefines::AnimId::Shoot && m_bodyParts.find(BodyPart::WeaponRanged) != m_bodyParts.end())
		m_sheath = BodyPart::WeaponRanged;

	if (m_animationId == UnitDefines::AnimId::Swing && m_bodyParts.find(BodyPart::Weapon) != m_bodyParts.end())
		m_sheath = BodyPart::Weapon;
}

void ClientUnit::setBodyPart(const string& modelName, const BodyPart bpart)
{
	if (auto ptr = sContentMgr->spawnModelScript(modelName))
	{
		unique_ptr<ModelScriptRender> oldPart = move(m_bodyParts[bpart]);
		m_bodyParts[bpart] = make_unique<ModelScriptRender>();
		m_bodyParts[bpart]->setModelScript(move(ptr));

		// Would fall out of sync with the other parts otherwise
		if (oldPart != nullptr)
			m_bodyParts[bpart]->sync(*oldPart);
	}
}

void ClientUnit::clearBodyPart(const BodyPart bpart)
{
	auto itr = m_bodyParts.find(bpart);

	if (itr != m_bodyParts.end())
		m_bodyParts.erase(itr);
}
		
void ClientUnit::parseAuraVector(const vector<GP_Server_UnitAuras::AuraInfo>& incomingAuras, vector<GP_Server_UnitAuras::AuraInfo>& myAuras)
{
	// Take away what no longer exists and register what does exist
	for (auto itr = myAuras.begin(); itr != myAuras.end();)
	{
		if (find_if(incomingAuras.begin(), incomingAuras.end(), [&](const GP_Server_UnitAuras::AuraInfo& s) { return s.spellId == itr->spellId; }) != incomingAuras.end())
		{
			// Exists in both
			++itr;
		}
		else
		{
			// Aura no longer exists
			auraAnimationDecrement(itr->spellId);
			itr = myAuras.erase(itr);
		}
	}

	for (auto itr = incomingAuras.begin(); itr != incomingAuras.end(); ++itr)
	{
		if (find_if(myAuras.begin(), myAuras.end(), [&](const GP_Server_UnitAuras::AuraInfo& s) { return s.spellId == itr->spellId; }) != myAuras.end())
		{
			// Exists in both
			;
		}
		else
		{
			// New aura
			auraAnimationIncrement(itr->spellId);
		}
	}

	myAuras = incomingAuras;
}

void ClientUnit::setBuffs(const vector<GP_Server_UnitAuras::AuraInfo>& auras)
{
	parseAuraVector(auras, m_buffs);
}
		
void ClientUnit::setDebuffs(const vector<GP_Server_UnitAuras::AuraInfo>& auras)
{
	parseAuraVector(auras, m_debuffs);
}
	
void ClientUnit::finishAnimation()
{
	if (m_npcModel != nullptr)
		m_npcModel->setToLastFrame(m_animationId);

	for (auto& itr : m_bodyParts)
		itr.second->setToLastFrame(m_animationId);
}

void ClientUnit::playAnimation(const UnitDefines::AnimId id)
{
	if (m_animationId == id)
	{
		if (m_npcModel != nullptr)
			m_npcModel->reset();

		for (auto& itr : m_bodyParts)
			itr.second->reset();

		m_animFreezeFrame = 0;
		m_animPlaybackSpeed = 0;
	}

	if (isAnimationPlayOnce(id))
		setAnimation(id);
}

float ClientUnit::getCombatVolume() const
{
	if (isLocal())
		return 1.f;

	return 0.5f;
}

void ClientUnit::setCastBar(shared_ptr<WorldSpellAnimation> sa, const int spellId, const int miliseconds)
{
	if (m_castAnimation != nullptr)
		m_castAnimation->cancel();

	m_castAnimation = sa;
	m_castSpellId = spellId;
	m_castStartTime = std::clock();
	m_castStopTime = std::clock() + miliseconds;
}

void ClientUnit::playSoundMelee(ClientUnit& victim, const SpellDefines::HitResult hitresult)
{
	if (hitresult == SpellDefines::HitResult::Miss || hitresult == SpellDefines::HitResult::Evade)
	{
		victim.playSound("dodge_default.wav", 1.f, getCombatVolume());
	}
	else if (hitresult == SpellDefines::HitResult::Dodge)
	{
		victim.playSound("swishverb4.ogg", 1.f, getCombatVolume());
	}
	else if (hitresult == SpellDefines::HitResult::Block)
	{
		victim.playSound(Util::randomChoice(string("e3_attack_hardhit01.ogg"), string("e3_attack_hardhit02.ogg"), string("e3_attack_hardhit03.ogg")), 1.f, getCombatVolume());
	}
	else if (hitresult == SpellDefines::HitResult::Parry)
	{
		victim.playSound("attack_metal_case.ogg", 1.f, getCombatVolume());
	}
	else if (hitresult == SpellDefines::HitResult::Normal || hitresult == SpellDefines::HitResult::Crit)
	{
		// Virtual, protected
		playSoundMelee(victim);
	}
}

bool ClientUnit::emitsLight(MapCellClient::LightSource* ls /*= nullptr*/) /*final*/
{
	if (ls != nullptr)
	{
		ls->power = 1.f;
		ls->scale =  0.5f;
		ls->screenPos = getScreenPosition();
	}
		
	if (getMap() != nullptr && !getMap()->outdoors())
		return isLocal() || faction() == UnitDefines::Friendly;

	return true;
}

void ClientUnit::chatMsg(const string& str, const ChatDefines::Channels c)
{
	m_overheads->setChatBubble(str, GameChat::getChatColor(c));
}

bool ClientUnit::isAnimationFinished() const
{
	if (!isAnimationPlayOnce(m_animationId))
		return false;

	if (auto modelScript = fetchFirstModelScript())
		return modelScript->done(m_animationId);

	return false;
}

bool ClientUnit::isAnimationPlayableWhileMoving(const UnitDefines::AnimId id) const
{
	switch (id)
	{
		case UnitDefines::AnimId::Cast:
		case UnitDefines::AnimId::CastAlt:
		case UnitDefines::AnimId::Stance:
			return false;
		case UnitDefines::AnimId::Spawn:
		case UnitDefines::AnimId::Run:
		case UnitDefines::AnimId::CritDie:
		case UnitDefines::AnimId::Swing:
		case UnitDefines::AnimId::Hit:
		case UnitDefines::AnimId::Block:
		case UnitDefines::AnimId::Shoot:
			return true;
	}

	return true;
}

bool ClientUnit::isAnimationPlayOnce(const UnitDefines::AnimId id) const
{
	if (auto modelScript = fetchFirstModelScript())
		return modelScript->isAnimationPlayOnce(id);

	return false;
}

ModelScriptRender const* ClientUnit::fetchFirstModelScript() const
{
	if (m_npcModel != nullptr)
		return m_npcModel.get();

	for (auto& itr : m_bodyParts)
	{
		if (itr.second != nullptr)
			return itr.second.get();
	}

	return nullptr;
}

/*static*/
UnitDefines::Direction ClientUnit::computeDirection(float orientation)
{
	static vector<UnitDefines::Direction> dirs =
	{
		UnitDefines::SouthEast,
		UnitDefines::South,
		UnitDefines::SouthWest,
		UnitDefines::West,
		UnitDefines::NorthWest,
		UnitDefines::North,
		UnitDefines::NorthEast,
		UnitDefines::East,
	};
	
	static float fullCircle = M_PI * 2.0f;
	static float piece = fullCircle / UnitDefines::NumDirections;
	orientation += piece / 4;

	if (orientation >= fullCircle)
		orientation -= floor(orientation / fullCircle) * fullCircle;
	
	if (orientation < 0)
		orientation = 0;

	const size_t idx = static_cast<size_t>(orientation / piece);
	ASSERT(idx < dirs.size());
	return dirs[idx];
}

sf::Color ClientUnit::getNameColor() const
{
	if (getType() == MutualObject::Npc && !isAlive())
		return sf::Color(150, 150, 150);

	sf::Color result = getFactionColor(faction());

	if (faction() == UnitDefines::Faction::Hostile && result == sf::Color::Red)
	{
		if (getWorld()->myself() != nullptr)
		{
			int playerLevel = getWorld()->myself()->getLevel();

			if (playerLevel > getLevel())
			{
				int levelDiff = playerLevel - getLevel();
				float factor;

				if (levelDiff >= PlayerDefines::Experience::MaxLevelDiffExp)
					factor = 1.0f;
				else if (levelDiff >= 3)
					factor = 0.8f + (levelDiff - 3) * 0.05f;
				else if (levelDiff == 2)
					factor = 0.65f;
				else
					factor = 0.5f;

				constexpr sf::Uint8 greyValue = 150;
				result = sf::Color(
					static_cast<sf::Uint8>(result.r + factor * (greyValue - result.r)),
					static_cast<sf::Uint8>(result.g + factor * (greyValue - result.g)),
					static_cast<sf::Uint8>(result.b + factor * (greyValue - result.b))
				);
			}
		}
	}

	return result;
}

/*static*/
sf::Color ClientUnit::getFactionColor(const UnitDefines::Faction fact)
{
	switch (fact)
	{
		case UnitDefines::Faction::Friendly: return sf::Color::Green;
		case UnitDefines::Faction::Hostile: return sf::Color::Red;
		case UnitDefines::Faction::PvP: return sf::Color(255, 128, 0, 255);
		case UnitDefines::Faction::Neutral: return sf::Color::Yellow;
		case UnitDefines::Faction::PlayerDefault: return sf::Color(0x45b6feff);
	}

	return sf::Color::White;
}

void ClientUnit::setCurrentCell(const int c, MapCellClient& cell) /*override*/
{
	const bool newcell = getCurrentCell() != c;
	
	__super::setCurrentCell(c, cell);

	if (!newcell || getMap() == nullptr)
		return;
	
	vector<string> footsteps;

	// Don't break out because we want the topmost to always win
	for (auto& cellsRef : getMap()->getFootstepCellsRef())
	{
		if (auto itr = static_cast<MapCellClient*>(getMap()->getCell(cellsRef.first)))
		{
			for (int layer = GameMap::Layer1; layer <= GameMap::Layer2; ++layer)
			{
				sf::Vector2i topleft;
				sf::Vector2i bottomright;
				sf::Vector2i screenPos(sf::Vector2f(cellsRef.second.x, cellsRef.second.y));

				itr->computeLayerBounds(screenPos, topleft, bottomright, layer);
						
				// If within the cells screen bounds
				if (getScreenPosition().x >= topleft.x && getScreenPosition().x <= bottomright.x &&
					getScreenPosition().y >= topleft.y && getScreenPosition().y <= bottomright.y)
				{
					sContentMgr->getFootstepsForTexture(itr->getTextureName(layer), footsteps);
				}	
			}
		}
	}

	if (footsteps.empty())
		registerFootstepSounds({ "step_grass1.ogg", "step_grass2.ogg", "step_grass3.ogg", "step_grass4.ogg" });	
	else
		registerFootstepSounds(footsteps);
}