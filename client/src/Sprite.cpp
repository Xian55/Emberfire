#include "stdafx.h"
#include "Sprite.h"
#include "Application.h"
#include "ContentMgr.h"
#include "ParticleSystem.h"
#include "SpriteScript.h"

#include <assert.h>

Sprite::Sprite(shared_ptr<sf::Texture> t, const string& textureName) :
	m_texture(t),
	m_textureName(textureName),
	m_blend(sf::BlendAlpha)
{
	if (t != nullptr)
		setTexture(*t);

	m_checkedScript = false;
}

Sprite::Sprite(Sprite& spr)
{
	// Can't copy sf::Sprite in the way you think.
	ASSERT(0);
}

Sprite::~Sprite()
{

}

void Sprite::render(sf::Vector2f pos, const float brightenPct /*= 0.f*/)
{
	if (m_hotspot.x != 0)
		pos.x -= getScale().x * (float)m_hotspot.x;

	if (m_hotspot.y != 0)
		pos.y -= getScale().y * (float)m_hotspot.y;

	setPosition(pos.x, pos.y);

	if (brightenPct != 0.f)
	{
		sf::RenderStates rs;
		rs.shader = &sContentMgr->getBrightShader();
		rs.blendMode = m_blend;
		sContentMgr->getBrightShader().setUniform("brightness", max(0.f, brightenPct));
		sContentMgr->getBrightShader().setUniform("alpha", float(getColor().a) / 255.f);
		sApplication->canvas().draw(*this, rs);
	}
	else
	{
		sApplication->canvas().draw(*this, m_blend);
	}
}

void Sprite::renderStretch(sf::Vector2f pos1, sf::Vector2f pos2)
{
	if (m_hotspot.x != 0)
	{
		pos1.x -= m_hotspot.x;
		pos2.x -= m_hotspot.x;
	}

	if (m_hotspot.y != 0)
	{
		pos1.y -= m_hotspot.y;
		pos2.y -= m_hotspot.y;
	}

	sf::RectangleShape rectangle;
	rectangle.setFillColor(getColor());
	rectangle.setTexture(getTexture());
	rectangle.setPosition(pos1);
	rectangle.setSize({ pos2.x - pos1.x, pos2.y - pos1.y });
	sApplication->canvas().draw(rectangle, m_blend);
}

void Sprite::renderAsCircle(const sf::Vector2f& pos, const int radius, const sf::Vector2i& textureRectOffset, const float outlineThick, const sf::Color outlineCol)
{
	if (getTexture() == nullptr)
		return;

	sf::CircleShape circle;
	circle.setPosition(pos.x - radius, pos.y - radius);
	circle.setTexture(getTexture());
	circle.setTextureRect({ textureRectOffset.x, textureRectOffset.y, (int)getTexture()->getSize().x, (int)getTexture()->getSize().x });
	circle.setRadius(float(radius));
	circle.setOutlineThickness(outlineThick);
	circle.setOutlineColor(outlineCol);

	sApplication->canvas().draw(circle, m_blend);
}

void Sprite::cacheRenderScript()
{
	if (m_checkedScript)
		return;

	m_checkedScript = true;
	m_renderScript = sContentMgr->getSpriteRenderScript(m_textureName);
}

void Sprite::renderScript(const sf::Vector2f& pos, const float skew, const bool particles, const float brightenPct)
{
	// A hotspot will now be assigned no matter what, therefore this is a way to say "only do this once"
	if (!hasHotspot())
	{
		cacheRenderScript();

		if (m_renderScript != nullptr)
		{
			setHotspot(m_renderScript->hotspot());

			if (!m_renderScript->psi().empty())
			{
				ASSERT(m_particleSystems.empty());

				for (auto& it : m_renderScript->psi())
				{
					if (auto ptr = sContentMgr->spawnParticleSystem(it.systemName))
						m_particleSystems.push_back(std::move(ptr));
				}
			}
		}

		if (!hasHotspot())
		{
			if (getTexture() != nullptr)
				setHotspot(sf::Vector2i(getTexture()->getSize().x / 2, static_cast<int>(float(getTexture()->getSize().y) / 1.25f)));
			else
				setHotspot({1, 1});
		}

		ASSERT(hasHotspot());
	}

	if (skew != 0)
		renderSkew(pos, skew);
	else
		render(pos, brightenPct);

	// We assume that there's a render script object if there's also a particle system object
	if (!m_particleSystems.empty() && particles)
	{
		ASSERT(m_renderScript->psi().size() == m_particleSystems.size());

		for (size_t i = 0; i < m_renderScript->psi().size(); ++i)
		{
			const auto& scriptInfo = m_renderScript->psi()[i];
			sf::Vector2f particlePos(pos.x, pos.y);

			particlePos.x += getScale().x * (float)scriptInfo.relativePos.x;
			particlePos.y += getScale().y * (float)scriptInfo.relativePos.y;

			particlePos.x -= getScale().x * (float)m_hotspot.x;
			particlePos.y -= getScale().y * (float)m_hotspot.y;

			m_particleSystems[i]->update(sApplication->getElapsed());
			m_particleSystems[i]->moveTo(particlePos, true);

			sApplication->canvas().draw(*m_particleSystems[i]);
		}
	}
}

void Sprite::renderSkew(sf::Vector2f pos, const float pct)
{
	if (getTexture() == nullptr)
		return;

	if (!getTexture()->isSmooth())
		const_cast<sf::Texture*>(getTexture())->setSmooth(true);

	if (m_hotspot.x != 0)
		pos.x -= m_hotspot.x;

	if (m_hotspot.y != 0)
		pos.y -= m_hotspot.y;

	const sf::Vector2u size = getTexture()->getSize();
	const float amount = float(size.x) * pct;

	sf::VertexArray box(sf::Quads, 4);

	// Screen position
	box[0].position = sf::Vector2f(pos.x + amount, pos.y);			// Top left corner position
	box[1].position = sf::Vector2f(pos.x, pos.y + size.y);			// Bottom left corner position
	box[2].position = sf::Vector2f(pos.x + size.x, pos.y + size.y);	// Bottom right corner position
	box[3].position = sf::Vector2f(pos.x + size.x + amount, pos.y);	// Top right corner position

	// Within the texture
	box[0].texCoords = sf::Vector2f(0, 0);
	box[1].texCoords = sf::Vector2f(0, static_cast<float>(size.y) - 1.0f);
	box[2].texCoords = sf::Vector2f(static_cast<float>(size.x) - 1.0f, static_cast<float>(size.y) - 1.0f);
	box[3].texCoords = sf::Vector2f(static_cast<float>(size.x) - 1.0f, 0);

	sApplication->canvas().draw(box, getTexture());
}

void Sprite::setHotspotEasy(const bool centerX, const bool centerY /*= false*/, const bool floorY /*= false*/)
{
	if (getTexture() == nullptr)
		return;

	sf::Vector2i result;

	if (centerX)
		result.x += static_cast<int>(static_cast<float>(getTexture()->getSize().x) / 2.0f);

	if (centerY)
		result.y += static_cast<int>(static_cast<float>(getTexture()->getSize().y) / 2.0f);

	if (floorY)
		result.y += static_cast<int>(static_cast<float>(getTexture()->getSize().y));

	setHotspot(result);
}

void Sprite::capSize(const float maxW, const float maxH)
{
	if (getTexture() == nullptr)
		return;

	const float w = getGlobalBounds().width;
	const float textureW = static_cast<float>(getTexture()->getSize().x);

	const float h = getGlobalBounds().height;
	const float textureH = static_cast<float>(getTexture()->getSize().y);

	if (w > maxW || h > maxH)
	{
		const float scaleX = maxW / textureW;
		const float scaleY = maxH / textureH;
		const float minscale = min(scaleX, scaleY);

		const sf::Vector2f sfScale({ minscale, minscale });

		if (getScale() != sfScale)
			setScale(sfScale);
	}
}
