#pragma once

class SpriteScript;
class ParticleSystem;

// The texture we use must not be destroyed as long as the sprite is still being used
class Sprite : public sf::Sprite
{
	public:
		Sprite(shared_ptr<sf::Texture> t, const string& textureName);
		Sprite(Sprite& spr);
		virtual ~Sprite();

		void render(sf::Vector2f pos, const float brightnessPct = 0.f);
		void renderStretch(sf::Vector2f pos1, sf::Vector2f pos2);
		void renderSkew(sf::Vector2f pos, const float pct);
		void renderScript(const sf::Vector2f& pos, const float skew = 0.0f, const bool particles = true, const float brightnessPct = 0.f);
		void renderAsCircle(const sf::Vector2f& pos, const int radius, const sf::Vector2i& textureRectOffset = {}, const float outlineThick = 0.f, const sf::Color outlineCol = sf::Color::White);

		void cacheRenderScript();
			
		void setBlendMode(const sf::BlendMode b) { m_blend = b; }
		void setHotspot(const sf::Vector2i hotspot) { m_hotspot = hotspot; }
		void setHotspotEasy(const bool centerX, const bool centerY = false, const bool floorY = false);
		void capSize(const float w, const float h);

		bool hasParticleSystems() const { return !m_particleSystems.empty(); }
		bool hasHotspot() const { return m_hotspot.x != 0 || m_hotspot.y != 0; }
		
		auto& getHotspot() const { return m_hotspot; }
		auto& getTextureName() const { return m_textureName; }

		sf::Vector2f vbounds() const { return { getGlobalBounds().width, getGlobalBounds().height }; }
		SpriteScript const* spriteScript() const { return m_renderScript.get(); }

	private:
		string m_textureName;
		sf::Vector2i m_hotspot;
		sf::BlendMode m_blend;

		bool m_checkedScript;

		shared_ptr<sf::Texture> m_texture;
		shared_ptr<SpriteScript> m_renderScript;

		vector<unique_ptr<ParticleSystem>> m_particleSystems;
};