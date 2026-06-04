#include "stdafx.h"
#include "WorldSpellAnimation.h"
#include "ContentMgr.h"
#include "ParticleSystem.h"
#include "Application.h"
#include "SpriteAnimation.h"
#include "ClientUnit.h"
#include "SpellVisualKit.h"

#include "..\Geo2d.h"
#include "..\StringHelpers.h"

#include "..\SqlConnector\QueryResult.h"

#include <assert.h>

WorldSpellAnimation::WorldSpellAnimation(ClientMap* clientMap, const int spellId, const bool casting, const bool useGo) :
	WorldRenderable(clientMap),
	m_traveling(!casting),
	m_orientation(0.f),
	m_castingExpiration(0),
	m_casting(casting),
	m_spellId(spellId),
	m_init(false),
	m_canceled(false),
	m_speed(0.f),
	m_go(useGo)
{
	
}

WorldSpellAnimation::~WorldSpellAnimation()
{
	cancel();
}

void WorldSpellAnimation::init()
{
	if (m_canceled)
	{
		setDone(true);
		return;
	}
	
	if (m_init)
		return;

	if (m_casting)
		initCasting();
	else if (m_go)
		initImpact();
	else
		initTravel();

	m_init = true;
}

/*virtual*/
void WorldSpellAnimation::update() /*final*/
{
	if (!m_init)
		init();
	
	if (m_casting)
	{
		if (m_sourceObj != nullptr && m_castingExpiration > std::clock())
		{
			if (ClientUnit* ptr = dynamic_cast<ClientUnit*>(m_sourceObj.get()))
			{
				if ((ptr->getCastSpellId() == m_spellId || ptr->getWorld() == nullptr) && ptr->getAnimation() != m_castingAnim)
					ptr->playAnimation(static_cast<UnitDefines::AnimId>(m_castingAnim));

				if (ptr->getCastSpellId() == m_spellId || ptr->getWorld() == nullptr)
				{
					ptr->setAnimPlaybackSpeed(m_animSpeed_Casting);
					ptr->setAnimFreezeFrame(m_animFreezeFrame_Casting);
				}
			}

			m_worldPosition = m_sourceObj->getWorldPosition();
		}
	}
	else if (!m_traveling)
	{
		if (m_targetObj != nullptr && !m_go)
			m_worldPosition = m_targetObj->getWorldPosition();
	
		if (!m_targetPos.isNull())
			m_worldPosition = m_targetPos;
	}

	if (m_travelingKit != nullptr)
	{ 
		m_travelingKit->update();
		m_travelingKit->movePsiTo(sf::Vector2f(getScreenPosition()), false);
	}

	if (m_castingKit != nullptr)
	{ 
		m_castingKit->update();
		m_castingKit->movePsiTo(sf::Vector2f(getScreenPosition()), true);
	}

	if (m_impactKit != nullptr)
	{ 
		m_impactKit->update();
		m_impactKit->movePsiTo(sf::Vector2f(getScreenPosition()), true);
	}

	// Model color (source)
	if (ClientUnit* ptr = dynamic_cast<ClientUnit*>(m_sourceObj.get()))
	{
		sf::Color glowColor;

		if (m_castingKit != nullptr && m_castingKit->getUnitGlowData(glowColor))
			ptr->registerModelColor(glowColor);
	}
	
	// Model color (target)
	if (ClientUnit* ptr = dynamic_cast<ClientUnit*>(m_targetObj.get()))
	{
		sf::Color glowColor;

		if ((m_impactKit != nullptr && m_impactKit->getUnitGlowData(glowColor)) ||
			(m_travelingKit != nullptr && m_travelingKit->getUnitGlowData(glowColor)))
		{
			ptr->registerModelColor(glowColor);
		}
	}

	__super::update();

	if (m_casting)
		updateCasting();
	else if (m_traveling)
		updateTravel();
}

/*virtual*/
void WorldSpellAnimation::render() /*final*/
{
	__super::render();

	ASSERT(m_cameraPtr != nullptr);
		
	// Keep rendering particles so it can finish dissolving
	if (m_travelingKit != nullptr)
	{
		sf::Vector2f transposePosition(m_cameraPtr->x - m_lastCameraPosition.x, m_cameraPtr->y - m_lastCameraPosition.y);
		m_travelingKit->transposePsi(transposePosition);
		m_travelingKit->drawPsi();
	}

	if (m_castingKit != nullptr)
		m_castingKit->drawPsi();

	if (m_impactKit != nullptr)
		m_impactKit->drawPsi();
	
	if (m_casting)
	{
		if (m_castingKit != nullptr)
		{
			m_castingKit->drawSprites(getScreenPosition(), m_orientation);

			if (m_castingKit->finished())
				m_castingKit = nullptr;
		}

		if (std::clock() > m_castingExpiration)
			cancel();

		if (finished())
			setDone(true);
	}
	else
	{
		if (m_travelingKit != nullptr)
		{
			m_travelingKit->drawSprites(getScreenPosition(), m_orientation);
			
			if (m_travelingKit->finished())
				m_travelingKit = nullptr;
		}

		if (m_impactKit != nullptr)
		{
			m_impactKit->drawSprites(getScreenPosition(), m_orientation);
			
			if (m_impactKit->finished())
				m_impactKit = nullptr;
		}

		if (finished())
			setDone(true);
	}

	m_lastCameraPosition = { m_cameraPtr->x, m_cameraPtr->y };
}

float WorldSpellAnimation::getAngle(const float x, const float y) const
{
	const float dx = x - m_worldPosition.x;
	const float dy = y - m_worldPosition.y;
	const float ang = atan2(dy, dx);
	return (ang >= 0) ? ang : 2.0f * M_PI + ang;
}

/*static*/
void WorldSpellAnimation::preLoad(const int spellId, vector<shared_ptr<SpriteAnimation>>& output)
{
	auto& sv = sContentMgr->db("spell_visual");
	auto& svk = sContentMgr->db("spell_visual_kit");

	auto dowork = [&](const int kitId, string keyname)
	{
		string spranim = svk.data(kitId, keyname);
			
		if (spranim.empty())
			return;

		if (auto anim = sContentMgr->spawnSpriteAnimation(spranim))
		{
			anim->load();
			output.push_back(anim);
		}
	};

	const int castingKit = atoi(sv.data(spellId, "casting_kit").c_str());
	const int travelingKit = atoi(sv.data(spellId, "traveling_kit").c_str());
	const int impactKit = atoi(sv.data(spellId, "impact_kit").c_str());	
	const int goKit = atoi(sv.data(spellId, "go_kit").c_str());	
	
	dowork(castingKit, "spranim");
	dowork(castingKit, "spranim_2");
	
	dowork(travelingKit, "spranim");
	dowork(travelingKit, "spranim_2");
	
	dowork(impactKit, "spranim");
	dowork(impactKit, "spranim_2");
	
	dowork(goKit, "spranim");
	dowork(goKit, "spranim_2");
}

void WorldSpellAnimation::initCasting()
{
	auto& sv = sContentMgr->db("spell_visual");
	auto& st = sContentMgr->db("spell_template");

	const int castingKit = atoi(sv.data(m_spellId, "casting_kit").c_str());

	m_castingKit = make_unique<SpellVisualKit>(castingKit, dynamic_cast<ClientObject*>(m_sourceObj.get()));
	m_castingKit->initSound(m_soundpoint_Source);
	m_castingAnim = atoi(sv.data(m_spellId, "unit_cast_animation").c_str());

	if (m_castingExpiration != 0)
	{
		m_castingKit->loop();
		m_castingKit->loopSound();
	}
	
	m_animSpeed_Casting = static_cast<float>(atof(sv.data(m_spellId, "uca_speed").c_str()));
	m_animFreezeFrame_Casting = atoi(sv.data(m_spellId, "uca_freeze_frame").c_str());
}

void WorldSpellAnimation::initTravel()
{
	auto& sv = sContentMgr->db("spell_visual");
	auto& st = sContentMgr->db("spell_template");
	const int travelingKit = atoi(sv.data(m_spellId, "traveling_kit").c_str());	

	m_travelingKit = make_unique<SpellVisualKit>(travelingKit, dynamic_cast<ClientObject*>(m_targetObj.get()));
	m_travelingKit->initSound(m_soundpoint_Source);
	m_travelingKit->loop();

	m_speed = static_cast<float>(atof(st.data(m_spellId, "speed").c_str()));
}

void WorldSpellAnimation::initImpact()
{
	auto& sv = sContentMgr->db("spell_visual");
	auto& st = sContentMgr->db("spell_template");
	const int impactKit = atoi(sv.data(m_spellId, m_go ? "go_kit" : "impact_kit").c_str());
	m_impactKit = make_unique<SpellVisualKit>(impactKit, dynamic_cast<ClientObject*>(m_sourceObj.get()));
	m_impactKit->initSound(m_soundpoint_Target);
	m_impactKit->stopPsi();
}

bool WorldSpellAnimation::finished() const
{
	if (m_traveling)
		return false;

	if (m_castingExpiration != 0 && std::clock() < m_castingExpiration)
		return false;
	
	if (m_castingKit != nullptr && !m_castingKit->finished())
		return false;
	
	if (m_travelingKit != nullptr && !m_travelingKit->finished())
		return false;
	
	if (m_impactKit != nullptr && !m_impactKit->finished())
		return false;

	return true;
}

bool WorldSpellAnimation::emitsLight(MapCellClient::LightSource* ls /*= nullptr*/) /*final*/
{
	if (ls != nullptr)
	{
		sf::Color col;

		if ((m_impactKit != nullptr && m_impactKit->getGroundGlowData(col)) ||
			(m_castingKit != nullptr && m_castingKit->getGroundGlowData(col)) || 
			(m_travelingKit != nullptr && m_travelingKit->getGroundGlowData(col)))
		{
			ls->power = 1.f;
			ls->scale = 0.5f;
			ls->screenPos = getScreenPosition();
			ls->color = col;
			ls->appearOnGround = true;
		}
	}

	return true;
}

void WorldSpellAnimation::updateCasting()
{
	if (m_castingKit == nullptr)
		return;

	if (m_castingExpiration != 0 && std::clock() >= m_castingExpiration)
	{
		m_castingExpiration = 0;
		m_castingKit->stop();
		
		auto& st = sContentMgr->db("spell_template");
		
		if (!Util::maskHas(_atoi64(st.data(m_spellId, "attributes").c_str()), SpellDefines::Attributes::DontStopCastingSound))
			m_castingKit->stopSound();
	}
}

void WorldSpellAnimation::updateTravel()
{
	// A speed of zero means we get there instantly
	if (m_speed == 0.0f && m_traveling)
	{
		impact();
		return;
	}

	if (sApplication->delta() == 0.0f)
		return;

	auto targetPos = deduceTargetPosition();
	auto newPos = Geo2d::extrude(m_worldPosition.x, m_worldPosition.y, targetPos.x, targetPos.y, m_speed * sApplication->delta());

	if (newPos.getDist({ targetPos.x, targetPos.y }) <= 0.0f)
	{
		impact();
		return;
	}

	// Set facing to point is based on old position -> new position, do before assign
	m_orientation = getAngle(newPos.x, newPos.y);
	m_worldPosition.x = newPos.x;
	m_worldPosition.y = newPos.y;
}

void WorldSpellAnimation::cancel()
{
	if (m_castingKit != nullptr)
		m_castingKit->cancel();

	if (m_travelingKit != nullptr)
		m_travelingKit->cancel();

	if (m_impactKit != nullptr)
		m_impactKit->cancel();

	m_impactKit = nullptr;
	m_travelingKit = nullptr;
	m_castingKit = nullptr;

	m_canceled = true;
}

void WorldSpellAnimation::setTimer(const int miliseconds)
{
	if (miliseconds == 0)
		return;

	m_castingExpiration = std::clock() + static_cast<clock_t>(miliseconds);
}
		
void WorldSpellAnimation::setTarget(const Geo2d::Vector2& targetPos)
{
	m_targetPos = targetPos;
}

void WorldSpellAnimation::setSource(shared_ptr<WorldRenderable> target)
{ 
	m_sourceObj = target; 

	if (m_sourceObj != nullptr)
		m_worldPosition = m_sourceObj->getWorldPosition();
}
		
void WorldSpellAnimation::setTarget(shared_ptr<WorldRenderable> target)
{
	m_targetObj = target;
}

Geo2d::Vector2 WorldSpellAnimation::deduceTargetPosition() const
{
	if (!m_targetPos.isNull())
		return m_targetPos;

	if (m_targetObj != nullptr)
		return m_targetObj->getWorldPosition();

	return m_targetPos;
}

/*virtual*/
void WorldSpellAnimation::initCamera() /*final*/
{
	ASSERT(m_cameraPtr != nullptr);
	m_lastCameraPosition = { m_cameraPtr->x, m_cameraPtr->y };
}

void WorldSpellAnimation::impact()
{
	if (m_travelingKit != nullptr)
	{
		m_travelingKit->stop();
		m_travelingKit->stopLoopingSound();
	}

	initImpact();
	m_traveling = false;
	m_worldPosition = deduceTargetPosition();
	computeScreenPosition();
}