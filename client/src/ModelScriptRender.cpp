#include "stdafx.h"
#include "ModelScriptRender.h"
#include "ModelScript.h"
#include "Application.h"
#include "ContentMgr.h"

#include <assert.h>

ModelScriptRender::ModelScriptRender() 
{
	reset();
		
	m_speed = 1.f;
	m_color = sf::Color::White;
	m_colorOverlay = sf::Color::White;
	m_overlayBlend = sf::BlendAdd;
}

ModelScriptRender::~ModelScriptRender()
{

}

void ModelScriptRender::setToLastFrame(const UnitDefines::AnimId animId)
{
	if (auto anim = m_script->getAnim(animId))
		m_currentFrame = anim->numFrames() - 1;
}

void ModelScriptRender::reset()
{
	m_currentFrame = 0;
	m_timer = 0.0f;
	m_reverse = false;
	m_timerTotal = 0.0f;
	m_freezeFrame = -1;
	m_timerRatio = 1.f;
}

void ModelScriptRender::setModelScript(unique_ptr<ModelScript> ptr)
{
	ASSERT(ptr != nullptr);
	m_script = move(ptr);	

	if (m_textureExists = sContentMgr->isTextureExist(m_script->getTextureName()))
		queueTextureLoad();

	reset();
}

void ModelScriptRender::sync(ModelScriptRender& other)
{
	m_reverse = other.m_reverse;
	m_timer = other.m_timer;
	m_currentFrame = other.m_currentFrame;
	m_timerTotal = other.m_timerTotal;
}

bool ModelScriptRender::forwardThenBack(const UnitDefines::AnimId animId) const
{
	return animId == UnitDefines::AnimId::Cast || animId == UnitDefines::AnimId::Hit || 
		animId == UnitDefines::AnimId::Block;
}

bool ModelScriptRender::done(const UnitDefines::AnimId animId) const
{
	if (m_script == nullptr)
		return true;

	if (auto anim = m_script->getAnim(animId))
	{
		if (forwardThenBack(animId))
		{
			float inverseFps = anim->durationInSeconds() / static_cast<float>(anim->numFrames());
			float expectedNumFrames = float((anim->numFrames() - 1) * 2);
			float dur = expectedNumFrames * inverseFps;
			float largeDur =  anim->durationInSeconds() * 2.f;

			// Play back -> forch, but just once
			return m_timerTotal >= dur;
		}

		return m_timerTotal >= anim->durationInSeconds();
	}

	return false;
}

bool ModelScriptRender::isAnimationPlayOnce(const UnitDefines::AnimId animId) const
{
	switch (animId)
	{
		case UnitDefines::AnimId::Stance:
		case UnitDefines::AnimId::Run:
			return false;
	}

	return true;

	//if (auto anim = m_script->getAnim(animId))
	//	return anim->type() == FlareAnimation::AnimType::PlayOnce;

	//return false;
}

void ModelScriptRender::queueTextureLoad()
{
	if (std::clock() - m_lastQueueTime < 5000)
		return;

	if (m_script != nullptr)
		sContentMgr->queueTextureLoad(m_script->getTextureName());

	m_lastQueueTime = std::clock();
}

sf::FloatRect ModelScriptRender::render(const sf::Vector2i& pos, const UnitDefines::AnimId animId, const UnitDefines::Direction dir, const bool forceLoop /*= false*/)
{
	if (m_script == nullptr)
		return sf::FloatRect{};

	if (m_texture == nullptr)
	{
		if (sContentMgr->isTextureLoaded(m_script->getTextureName()))
			m_texture = sContentMgr->getTexture(m_script->getTextureName());
		else
			queueTextureLoad();

		return sf::FloatRect{};
	}

	//m_texture->setSmooth(true);
	
	// Must be positive or not at all
	if (m_timerRatio <= 0)
		m_timerRatio = 1.f;

	m_timer += sApplication->delta() * m_timerRatio * m_speed;
	m_timerTotal += sApplication->delta() * m_timerRatio * m_speed;

	auto anim = m_script->getAnim(animId);

	// Some NPC models only have an idle animation
	if (anim == nullptr)
		anim = m_script->getAnim(UnitDefines::AnimId::Stance);

	if (anim != nullptr)
	{
		ASSERT(anim->numFrames() > 0);
		const float inverseFps = anim->durationInSeconds() / static_cast<float>(anim->numFrames());
	
		if (m_timer >= inverseFps)
		{
			m_timer = 0.0f;

			if (m_reverse)
			{
				if (m_currentFrame > 0)
					--m_currentFrame;
			}
			else
			{
				++m_currentFrame;
			}
		}

		if (m_currentFrame >= anim->numFrames() || (m_reverse && m_currentFrame == 0))
		{
			if (anim->type() == FlareAnimation::BackForth || (forceLoop && anim->type() == FlareAnimation::PlayOnce) || forwardThenBack(animId))
			{
				if (anim->numFrames() > 1)
					m_currentFrame = anim->numFrames() - 2;
				else
					m_currentFrame = anim->numFrames() - 1;

				m_reverse = !m_reverse;
			}
			else if (anim->type() == FlareAnimation::PlayOnce)
			{
				m_currentFrame = anim->numFrames() - 1;
			}
			else if (anim->type() == FlareAnimation::Loop)
			{
				m_currentFrame = 0;
			}
		}

		// Implement freeze frame parameter
		if (m_freezeFrame > 0)
		{
			if (m_currentFrame > (size_t)m_freezeFrame)
				m_currentFrame = (size_t)m_freezeFrame;

			m_timerTotal = 0;
		}

		ASSERT(m_currentFrame < anim->numFrames());

		auto frame = anim->getFrame(m_currentFrame, dir);
		
		// Some NPC's only have one direction animation
		if (frame->spriteRect.width <= 0)
			frame = anim->getFrame(m_currentFrame, UnitDefines::Direction::West);
		
		if (frame->spriteRect.width > 0)
		{
			auto renderOffset = sf::Vector2f(frame->renderOffset) * m_scale;
			sf::Vector2f sprPos(sf::Vector2f(pos) - renderOffset);

			sf::Sprite spr;
			spr.setPosition(sprPos);
			spr.setColor(m_color);
			spr.setTextureRect(frame->spriteRect);
			spr.setTexture(*m_texture);
			spr.setScale(m_scale, m_scale);

			sf::RenderStates rs;

			if (m_brightenPct > 0)
			{
				rs.shader = &sContentMgr->getBrightShader();
				sContentMgr->getBrightShader().setUniform("brightness", m_brightenPct);
				sContentMgr->getBrightShader().setUniform("alpha", float(m_color.a) / 255.f);
			}

			sApplication->canvas().draw(spr, rs);

			if (m_colorOverlay.toInteger() != defaultOverlayColor().toInteger())
			{
				sf::Color colorOverlay(m_colorOverlay);

				rs.shader = &sContentMgr->getRecolorShader();
				rs.blendMode = m_overlayBlend;

				sContentMgr->getRecolorShader().setUniform("colorR", float(colorOverlay.r) / 255.f);
				sContentMgr->getRecolorShader().setUniform("colorG", float(colorOverlay.g) / 255.f);
				sContentMgr->getRecolorShader().setUniform("colorB", float(colorOverlay.b) / 255.f);
				sContentMgr->getRecolorShader().setUniform("colorA", float(colorOverlay.a) / 255.f);

				sApplication->canvas().draw(spr, rs);
			}

			return spr.getGlobalBounds();
		}
	}

	return sf::FloatRect{};
}

auto ModelScriptRender::getNumFrames(const UnitDefines::AnimId animId) const
{
	if (m_script == nullptr)
		return size_t(0);

	return m_script->getAnim(animId)->numFrames();
}

