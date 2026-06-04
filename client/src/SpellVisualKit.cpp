#include "stdafx.h"
#include "SpellVisualKit.h"
#include "SpriteAnimation.h"
#include "ClientUnit.h"
#include "ParticleSystem.h"
#include "Application.h"
#include "ContentMgr.h"
#include "ClientObjectOverhead.h"

#include "..\StringHelpers.h"
#include "..\SqlConnector\QueryResult.h"

SpellVisualKit::SpellVisualKit(const int kitId, ClientObject const* obj) :
	m_obj(obj),
	m_ageSeconds(0.f),
	m_duration(1.f),
	m_kitId(kitId)
{
	auto& sv = sContentMgr->db("spell_visual");
	auto& svk = sContentMgr->db("spell_visual_kit");

	{
		// Sprite animation 1
		const string spranim = svk.data(kitId, "spranim");
		m_sprAnim[0].offset.x = keywords(svk.data(kitId, "spranim_x"));
		m_sprAnim[0].offset.y = keywords(svk.data(kitId, "spranim_y"));
		m_sprAnim[0].blendMode = BlendMode(atoi(svk.data(kitId, "spranim_blend").c_str()));
		m_sprAnim[0].topmost = bool(atoi(svk.data(kitId, "spranim_topmost").c_str()));

		if (!spranim.empty())
		{
			if (m_sprAnim[0].anim = sContentMgr->spawnSpriteAnimation(spranim))
			{
				m_sprAnim[0].anim->load();
				m_sprAnim[0].anim->setColor(sf::Color(strtoul(svk.data(kitId, "sprcolor").c_str(), nullptr, 10)));
				m_duration = max(m_duration, m_sprAnim[0].anim->durationInSeconds());
			}
		}
	}

	{
		// Sprite animation 2
		const string spranim2 = svk.data(kitId, "spranim_2");
		m_sprAnim[1].offset.x = keywords(svk.data(kitId, "spranim_x_2"));
		m_sprAnim[1].offset.y = keywords(svk.data(kitId, "spranim_y_2"));
		m_sprAnim[1].blendMode = BlendMode(atoi(svk.data(kitId, "spranim2_blend").c_str()));
		m_sprAnim[1].topmost = bool(atoi(svk.data(kitId, "spranim2_topmost").c_str()));

		if (!spranim2.empty())
		{
			if (m_sprAnim[1].anim = sContentMgr->spawnSpriteAnimation(spranim2))
			{
				m_sprAnim[1].anim->load();
				m_sprAnim[1].anim->setColor(sf::Color(strtoul(svk.data(kitId, "sprcolor_2").c_str(), nullptr, 10)));
				m_duration = max(m_duration, m_sprAnim[1].anim->durationInSeconds());
			}
		}
	}
	
	// Traveling animation by direction
	const string dirTravelingSprAnim = svk.data(kitId, "directional_spranim");

	if (!dirTravelingSprAnim.empty())
	{
		auto setDirAnim = [&](shared_ptr<SpriteAnimation>& anim, string dirTravelingSprAnim, const string& dirName)
		{
			Util::strReplaceAll(dirTravelingSprAnim, "$d", dirName);

			if (anim = sContentMgr->spawnSpriteAnimation(dirTravelingSprAnim))
			{
				anim->setForceLoop(true);
				m_duration = max(m_duration, anim->durationInSeconds());
			}
		};
		
		setDirAnim(m_travelingAnimDir[UnitDefines::West].anim, dirTravelingSprAnim, "west");
		setDirAnim(m_travelingAnimDir[UnitDefines::NorthWest].anim, dirTravelingSprAnim, "north_west");
		setDirAnim(m_travelingAnimDir[UnitDefines::North].anim, dirTravelingSprAnim, "north");
		setDirAnim(m_travelingAnimDir[UnitDefines::NorthEast].anim, dirTravelingSprAnim, "north_east");
		setDirAnim(m_travelingAnimDir[UnitDefines::East].anim, dirTravelingSprAnim, "east");
		setDirAnim(m_travelingAnimDir[UnitDefines::SouthEast].anim, dirTravelingSprAnim, "south_east");
		setDirAnim(m_travelingAnimDir[UnitDefines::South].anim, dirTravelingSprAnim, "south");
		setDirAnim(m_travelingAnimDir[UnitDefines::SouthWest].anim, dirTravelingSprAnim, "south_west");

		for (auto& itr : m_travelingAnimDir)
		{
			itr.second.offset.x = keywords(svk.data(kitId, "directional_spranim_x"));
			itr.second.offset.y = keywords(svk.data(kitId, "directional_spranim_y"));
		}
	}
	
	// Particle system
	const string psystem = svk.data(kitId, "psystem");
	m_psiOffset.x = keywords(svk.data(kitId, "psystem_x"));
	m_psiOffset.y = keywords(svk.data(kitId, "psystem_y"));

	if (psystem.size() > 1)
		m_psi = sContentMgr->spawnParticleSystem(psystem);

	m_groundGlowColor = _atoi64(svk.data(kitId, "ground_glow_color").c_str());
	m_unitGlowColor = _atoi64(svk.data(kitId, "unit_glow_color").c_str());
	m_waitForSprAnims = atoi(svk.data(kitId, "finish_sprite_anims").c_str());
}

SpellVisualKit::~SpellVisualKit()
{
	if (m_soundsf != nullptr)
		m_soundsf->setLoop(false);
}
		
void SpellVisualKit::initSound(const sf::Vector2f& screenPosition)
{
	auto& svk = sContentMgr->db("spell_visual_kit");
	m_sound = svk.data(m_kitId, "sound");

	if (!m_sound.empty())
		m_soundsf = sContentMgr->playSound(m_sound, 1.f, screenPosition);
}

int SpellVisualKit::keywords(const string& val)
{
	if (m_obj != nullptr && (val.find("h") != string::npos || val.find("n") != string::npos))
	{
		string newstr = val;
		Util::strReplaceAll(newstr, "height", to_string(m_obj->mouseableHeight()));

		if (auto unit = dynamic_cast<ClientUnit const*>(m_obj))
			Util::strReplaceAll(newstr, "name", to_string(unit->getOverhead()->getNameHeight()));

		auto result = Util::parseIntExpression(newstr);
		return result;
	}

	return atoi(val.c_str());
}

void SpellVisualKit::deletePsi()
{
	m_psi = nullptr;
}

void SpellVisualKit::stopSound()
{
	if (m_soundsf != nullptr)
	{
		m_soundsf->setLoop(false);
		m_soundsf->stop();
	}
}

void SpellVisualKit::stopLoopingSound()
{
	if (m_soundsf != nullptr)
		m_soundsf->setLoop(false);
}

void SpellVisualKit::stop()
{
	m_travelingAnimDir.clear();

	for (int i = 0; i < 2; ++i)
		m_sprAnim[i].anim = nullptr;

	if (m_psi != nullptr)
		m_psi->stop();
}

void SpellVisualKit::cancel()
{
	if (m_soundsf != nullptr)
		m_soundsf->stop();
	
	m_travelingAnimDir.clear();

	for (int i = 0; i < 2; ++i)
		m_sprAnim[i].anim = nullptr;

	m_psi = nullptr;
}

void SpellVisualKit::loopSound()
{
	if (m_soundsf != nullptr)
		m_soundsf->setLoop(true);
}

void SpellVisualKit::loop()
{
	for (auto& itr : m_travelingAnimDir)
	{
		if (itr.second.anim != nullptr)
			itr.second.anim->setForceLoop(true);
	}

	for (int i = 0; i < 2; ++i)
	{
		if (m_sprAnim[i].anim != nullptr)
			return m_sprAnim[i].anim->setForceLoop(true);
	}
}

void SpellVisualKit::updateOffsets()
{
	auto& sv = sContentMgr->db("spell_visual");
	auto& svk = sContentMgr->db("spell_visual_kit");
	
	m_sprAnim[0].offset.x = keywords(svk.data(m_kitId, "spranim_x"));
	m_sprAnim[0].offset.y = keywords(svk.data(m_kitId, "spranim_y"));
	
	m_sprAnim[1].offset.x = keywords(svk.data(m_kitId, "spranim_x_2"));
	m_sprAnim[1].offset.y = keywords(svk.data(m_kitId, "spranim_y_2"));
	
	m_psiOffset.x = keywords(svk.data(m_kitId, "psystem_x"));
	m_psiOffset.y = keywords(svk.data(m_kitId, "psystem_y"));
}

void SpellVisualKit::stopPsi()
{
	if (m_psi != nullptr)
	{
		if (!m_psi->stopped())
		{
			// Let glow start ticking down
			m_duration = 1.f;
			m_ageSeconds = 0.5f;
		}

		m_psi->stop();
	}
}

void SpellVisualKit::movePsiTo(const sf::Vector2f& pos, const bool moveParticles)
{
	if (m_psi != nullptr)
		m_psi->moveTo(pos + sf::Vector2f(m_psiOffset), moveParticles);
}

void SpellVisualKit::update()
{
	if (sApplication->isAudioDeviceChangedThisTick())
	{
		if (m_soundsf != nullptr)
		{
			m_soundsf->stop();
			m_soundsf = nullptr;
		}
	}

	updateOffsets();

	if (m_psi != nullptr)
		m_psi->update(sApplication->getElapsed());

	m_ageSeconds += sApplication->delta();
}

bool SpellVisualKit::getUnitGlowData(sf::Color& outputColor)
{
	if (m_unitGlowColor <= 0)
		return false;

	float alphaPct = getGlowAlpha();
	outputColor = sf::Color(uint32_t(m_unitGlowColor));
	outputColor.a = uint8_t(float(outputColor.a) * alphaPct);
	return true;
}

bool SpellVisualKit::getGroundGlowData(sf::Color& outputColor)
{
	if (m_groundGlowColor <= 0)
		return false;

	float alphaPct = getGlowAlpha();
	outputColor = sf::Color(uint32_t(m_groundGlowColor));
	outputColor.a = uint8_t(float(outputColor.a) * alphaPct);
	return true;
}

float SpellVisualKit::getGlowAlpha() const
{
	float durationPct = m_ageSeconds / m_duration;
	float alphaPct = 0.5f;

	if (durationPct <= 0.5f)
		alphaPct = durationPct / 0.5f;
	else
	{
		if (m_psi != nullptr && !m_psi->stopped())
			alphaPct = 1.f;
		else
			alphaPct = 1.f - ((durationPct - 0.5f) / 0.5f);
	}

	if (alphaPct < 0)
		alphaPct = 0;

	if (alphaPct > 1.f)
		alphaPct = 1.f;

	return alphaPct;
}

void SpellVisualKit::drawPsi()
{
	if (m_psi != nullptr)
		sApplication->canvas().draw(*m_psi);
}

void SpellVisualKit::transposePsi(const sf::Vector2f& transposePos)
{
	if (m_psi != nullptr)
		m_psi->transpose(transposePos.x, transposePos.y);
}

/*static*/
sf::BlendMode SpellVisualKit::getSfBlend(const BlendMode bm)
{
	switch (bm)
	{
		case BlendMode::BlendAdd: return sf::BlendAdd;
		case BlendMode::BlendMultiply: return sf::BlendMultiply;
		case BlendMode::BlendNone: return sf::BlendNone;
		case BlendMode::BlendScreen: return sf::BlendMode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::OneMinusSrcColor); 
	}

	return sf::BlendAlpha;
}

void SpellVisualKit::drawSprites(const sf::Vector2i& screenPosition, const float orientation)
{
	// Don't render until both are ready, or if neither are ready then may as well just call render() to ensure loading starts
	bool sameLoadState = m_sprAnim[0].anim == nullptr || m_sprAnim[1].anim == nullptr || m_sprAnim[0].anim->loadedEnough() == m_sprAnim[1].anim->loadedEnough();
	
	if (sameLoadState)
	{
		for (int i = 0; i < 2; ++i)
		{
			if (m_sprAnim[i].anim != nullptr && !m_sprAnim[i].anim->finished())
			{
				sf::BlendMode bm = getSfBlend(m_sprAnim[i].blendMode);
				m_sprAnim[i].anim->setBlendMode(bm);

				if (m_sprAnim[i].topmost && m_obj->getMap() != nullptr)
					m_obj->getMap()->registerTopmostSpriteAnim(m_sprAnim[i].anim, screenPosition + m_sprAnim[i].offset);
				else
					m_sprAnim[i].anim->render(screenPosition + m_sprAnim[i].offset);
			}
		}
	}

	auto dir = ClientUnit::computeDirection(orientation);
	auto& animDir = m_travelingAnimDir[dir];

	if (auto& anim = animDir.anim)
	{
		if (animDir.topmost && m_obj->getMap() != nullptr)
			m_obj->getMap()->registerTopmostSpriteAnim(anim, screenPosition + animDir.offset);
		else
			anim->render(screenPosition + animDir.offset);
	}
}

bool SpellVisualKit::hasSprAnimsPlaying() const
{
	for (int i = 0; i < 2; ++i)
	{
		if (m_sprAnim[i].anim != nullptr && !m_sprAnim[i].anim->finished())
			return true;
	}

	return false;
}

bool SpellVisualKit::waitForSprAnims() const 
{
	return m_waitForSprAnims; 
}

bool SpellVisualKit::finished() const
{	
	if (m_psi != nullptr)
	{
		if (!m_psi->finished())
			return false;
	}

	for (int i = 0; i < 2; ++i)
	{
		if (m_sprAnim[i].anim != nullptr && !m_sprAnim[i].anim->finished())
			return false;
	}
	
	for (auto& itr : m_travelingAnimDir)
	{
		if (itr.second.anim != nullptr && !itr.second.anim->finished())
			return false;
	}

	return true;
}