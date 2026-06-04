#pragma once

#include "ClientMap.h"

#include "..\Shared\MapCellT.h"

class Sprite;
class WorldRenderable;

class MapCellClient : public MapCellT
{
	public:
		struct LightSource
		{
			sf::Color color;
			sf::Vector2i screenPos;
			float power{0};
			float scale{0};
			bool appearAboveAll{0};
			bool appearOnGround{0};
		};

		struct Layer
		{
			bool renderOnce{0};
			shared_ptr<Sprite> sprite;
			shared_ptr<string> textureName;
			shared_ptr<ClientMap::Foliage> foliage;
		};

	public:
		MapCellClient(const int numLayers);
		virtual ~MapCellClient();
		
		void loadSprites();
		void unloadSprites();
		
		void clearSprite(const int layer);
		void refreshSpriteScale(const int layer);
		void renderLayer(const sf::Vector2f& position, const int layer, ClientMap& owner, const bool allowSkew = true, const bool allowWorldObjs = true);
		void setTexture(shared_ptr<string> textureName, const int layer);
		void addWorldRenderable(shared_ptr<WorldRenderable> wo);
		void eraseWorldRenderable(shared_ptr<WorldRenderable> wo);
		void computeLayerBounds(const sf::Vector2i& position, sf::Vector2i& topleft, sf::Vector2i& bottomright, const int layer) const;

		bool hasData() const;
		bool hasDataForLayer(const int layer) const;
		bool hasTextureForLayer(const int layer) const;
		bool hasFootsteps() const { return m_hasFootsteps; }
		bool emitsLight(vector<LightSource>& output, sf::Vector2i& screenPos);
		bool emitsLightCached() const { return m_emitsLight; }

		bool isLoaded() const { return m_loaded; }

		int getMaxHeight() const { return m_maxHeight; }
		int getMaxWidth() const { return m_maxWidth; }

		const string getTextureName(const int layer) const;

		const auto& getRenderablesRef() const { return m_worldRenderables; }

	private:
		void crunchDimensions();
		
		int m_maxHeight;
		int m_maxWidth;
		
		bool m_loaded;
		bool m_emitsLight;
		bool m_hasFootsteps;

		vector<Layer> m_layers;
		set<shared_ptr<WorldRenderable>> m_worldRenderables;
};

