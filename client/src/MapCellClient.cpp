#include "stdafx.h"
#include "MapCellClient.h"
#include "ContentMgr.h"
#include "ClientMap.h"
#include "Sprite.h"
#include "WorldRenderable.h"
#include "WorldSpellAnimation.h"
#include "SpriteScript.h"

#include <assert.h>

MapCellClient::MapCellClient(const int numLayers) :
	m_maxHeight(0),
	m_maxWidth(0),
	m_loaded(false),
	m_emitsLight(false),
	m_hasFootsteps(false)
{
	m_layers.resize(numLayers);
}

MapCellClient::~MapCellClient()
{

}

void MapCellClient::loadSprites()
{
	if (m_loaded)
		return;

	for (size_t l = 0; l < m_layers.size(); ++l)
	{
		auto& itr = m_layers[l];

		if (itr.textureName != nullptr)
		{
			itr.sprite = sContentMgr->spawnSprite(*itr.textureName);

			if (getLayerScale(l) != 0)
				refreshSpriteScale(l);

			if (l <= GameMap::Layer2)
				m_hasFootsteps = sContentMgr->isFootstepsForTexture(*itr.textureName);
		}
	}
	
	crunchDimensions();
	m_loaded = true;
}

void MapCellClient::unloadSprites()
{
	if (!m_loaded)
		return;

	for (auto& itr : m_layers)
		itr.sprite = nullptr;

	m_loaded = false;
}

void MapCellClient::computeLayerBounds(const sf::Vector2i& position, sf::Vector2i& topleft, sf::Vector2i& bottomright, const int layer) const
{	
	if (m_layers[layer].sprite != nullptr)
	{
		sf::Vector2i hotspot = m_layers[layer].sprite->getHotspot();
		topleft = position - hotspot;
		bottomright = topleft + sf::Vector2i(sf::Vector2f(m_layers[layer].sprite->getGlobalBounds().width, m_layers[layer].sprite->getGlobalBounds().height));
	}
}

void MapCellClient::renderLayer(const sf::Vector2f& position, const int l, ClientMap& owner, const bool allowSkew /*= true*/, const bool allowWorldObjs /*= true*/)
{
	auto& layer = m_layers[l];

	if (auto& sprite = layer.sprite)
	{
		if (!layer.renderOnce)
		{
			layer.foliage = owner.getFoliageRef(sprite->getTextureName());
			layer.renderOnce = true;
		}

		float skew = 0.f;

		if (allowSkew)
		{
			if (auto foliage = layer.foliage.get())
			{
				if (const auto result = owner.calcFoliageSkew(*layer.foliage))
					skew = result;
			}
		}

		sprite->renderScript(position, skew, allowWorldObjs);

		if (sprite->spriteScript() != nullptr && !sprite->spriteScript()->proximitySound().empty() && allowWorldObjs)
			owner.registryProximitySound(position, sprite->spriteScript()->proximitySound(), sprite->spriteScript()->promixitySoundDistFactor());
	}

	// Not sure yet if these should come after sprites
	if (!m_worldRenderables.empty() && allowWorldObjs)
	{
		map<int, vector<WorldRenderable*>> sortedObjs;
		map<int, vector<WorldRenderable*>> sortedAnims;

		auto itr = m_worldRenderables.begin();

		while (itr != m_worldRenderables.end())
		{
			auto& rdr = (*itr);

			if (!rdr->isRenderedOnLayer(ClientMap::Defines(l)))
			{
				++itr;
				continue;
			}

			if (rdr->done())
			{
				itr = m_worldRenderables.erase(itr);
			}
			else
			{
				if (auto anim = dynamic_pointer_cast<WorldSpellAnimation>(rdr))
					sortedAnims[anim->getScreenPosition().y].push_back(anim.get());
				else
					sortedObjs[rdr->getScreenPosition().y].push_back(rdr.get());

				++itr;
			}
		}

		for (auto& maptr : sortedObjs)
		{
			for (auto& obj : maptr.second)
				obj->render();
		}

		for (auto& maptr : sortedAnims)
		{
			for (auto& obj : maptr.second)
				obj->render();
		}
	}
}

void MapCellClient::clearSprite(const int l)
{
	auto& layer = m_layers[l];
	layer.renderOnce = false;
	layer.foliage = nullptr;
	layer.sprite = nullptr;
	layer.textureName = nullptr;
	m_loaded = false;
	crunchDimensions();
}

void MapCellClient::addWorldRenderable(shared_ptr<WorldRenderable> wo)
{
	m_worldRenderables.insert(wo);
}

void MapCellClient::eraseWorldRenderable(shared_ptr<WorldRenderable> wo)
{
	m_worldRenderables.erase(wo);
}

void MapCellClient::setTexture(shared_ptr<string> textureName, const int l)
{
	if (textureName == nullptr || textureName->empty())
		return;

	auto& layer = m_layers[l];
	layer.renderOnce = false;
	layer.foliage = nullptr;
	layer.textureName = textureName;
	m_loaded = false;
	return;
}

void MapCellClient::refreshSpriteScale(const int l)
{
	auto& layer = m_layers[l];

	if (auto& sprite = layer.sprite)
	{
		const float layerscale = getLayerScale(l);

		if (layerscale < 0.f)
			sprite->setScale(layerscale, abs(layerscale));
		else
			sprite->setScale(layerscale, layerscale);
	}
}

bool MapCellClient::hasDataForLayer(const int layer) const
{
	if (static_cast<size_t>(layer) >= m_layers.size())
		return false;

	if (!m_worldRenderables.empty())
		return true;

	return m_layers[layer].textureName != nullptr;
}

bool MapCellClient::emitsLight(vector<LightSource>& output, sf::Vector2i& screenPos)
{
	auto& layer = m_layers[GameMap::Layer3];

	if (m_worldRenderables.empty() && layer.sprite == nullptr)
		return m_emitsLight = false;

	bool emitsLight  = false;

	if (!m_worldRenderables.empty())
	{
		for (auto& itr : m_worldRenderables)
		{
			LightSource ls;
			ls.appearAboveAll = false;
			ls.appearOnGround = false;
			ls.power = 0.5f;
			ls.scale = 0.3f;
			ls.screenPos = itr->getScreenPosition();

			if (itr->emitsLight(&ls))
			{
				emitsLight = true;
				output.push_back(ls);
			}
		}
	}

	if (layer.sprite != nullptr)
	{
		layer.sprite->cacheRenderScript();

		if (layer.sprite->spriteScript() != nullptr && layer.sprite->spriteScript()->lightIntensity() > 0)
		{
			LightSource ls;
			ls.power = float(layer.sprite->spriteScript()->lightIntensity()) / 100.f;
			ls.scale = layer.sprite->spriteScript()->lightScale();
			ls.color = 	layer.sprite->spriteScript()->lightColor();
			ls.appearAboveAll = layer.sprite->spriteScript()->lightAboveAll();
			ls.appearOnGround = layer.sprite->spriteScript()->lightOnGround();
			ls.screenPos = screenPos + layer.sprite->spriteScript()->lightOffset();		
			output.push_back(ls);
			emitsLight = true;
		}
	}

	return m_emitsLight = emitsLight;
}

bool MapCellClient::hasTextureForLayer(const int layer) const
{
	if (static_cast<size_t>(layer) >= m_layers.size())
		return false;

	return m_layers[layer].textureName != nullptr;
}

void MapCellClient::crunchDimensions()
{
	m_maxHeight = 0;
	m_maxWidth = 0;

	for (auto& itr : m_layers)
	{
		if (itr.sprite != nullptr)
		{
			m_maxWidth = max(m_maxWidth, static_cast<int>(itr.sprite->getGlobalBounds().width));
			m_maxHeight = max(m_maxHeight, static_cast<int>(itr.sprite->getGlobalBounds().height));
		}
	}
}

const string MapCellClient::getTextureName(const int layer) const
{
	ASSERT(static_cast<size_t>(layer) < m_layers.size());

	if (m_layers[layer].textureName == nullptr)
		return "";

	return *m_layers[layer].textureName;
}

bool MapCellClient::hasData() const
{
	if (getFlags() != 0)
		return true;

	for (auto& itr : m_layers)
	{
		if (itr.textureName != nullptr)
			return true;
	}

	return false;
}