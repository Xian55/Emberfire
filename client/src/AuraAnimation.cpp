#include "stdafx.h"
#include "AuraAnimation.h"
#include "ContentMgr.h"
#include "SpriteAnimation.h"
#include "ClientObject.h"
#include "ParticleSystem.h"
#include "Application.h"
#include "SpellVisualKit.h"

#include "..\SqlConnector\QueryResult.h"

#include <assert.h>

AuraAnimation::AuraAnimation(const int auraKitId, ClientObject const& holder) :
	m_holder(holder),
	m_finished(false),
	m_stopped(false),
	m_kitId(auraKitId)
{

}

AuraAnimation::~AuraAnimation()
{

}

void AuraAnimation::render()
{
	if (m_finished)
		return;

	const sf::Vector2i screenPos = m_holder.getScreenPosition();
	
	m_kit->movePsiTo(sf::Vector2f(screenPos), true);
	m_kit->drawPsi();
	m_kit->drawSprites(screenPos, 0.f);
}

void AuraAnimation::stop()
{
	m_stopped = true;
	m_kit->stop();
	m_kit->stopSound();
}

void AuraAnimation::resume()
{
	m_stopped = false;
	m_kit = make_unique<SpellVisualKit>(m_kitId, &m_holder);

	if (m_holderPosForVolume)
		m_kit->initSound(sf::Vector2f(m_holder.getScreenPosition()));
	else
		m_kit->initSound();

	m_kit->loop();
}

bool AuraAnimation::finished() const
{
	if (m_kit != nullptr && !m_kit->finished())
		return false;

	return true;
}

void AuraAnimation::update()
{	
	if (m_finished)
		return;

	if (m_kit != nullptr)
		m_kit->update();

	if (finished())
		m_finished = true;
}
