#pragma once

class SpriteScript
{
	public:
		struct SpritePSI
		{
			string systemName;
			sf::Vector2i relativePos;
		};

	public:
		SpriteScript();
		~SpriteScript();

		void setDummyTexture(const bool v) { m_dummyTexture = v; }
		void setPromixitySoundDistFactor(const float v) { m_promixitySoundDistFactor = v; }
		void setProximitySound(const string& filename) { m_proximitySound = filename; }
		void setHotspot(const string& scriptname, const int x, const int y);
		void setHotspot(const int x, const int y);
		void editHotspot(const string& scriptname, const int xChange, const int yChange);
		void setPsi(const string& filename, const sf::Vector2i& pos);
		void setLight(const sf::Color& color, const int x_offset, const int y_offset, const int intensity, const bool lightOnGround, const bool lightAboveAll, float lightScale);

		const auto& hotspot() const { return m_hotspot; }
		const auto& psi() const { return m_psi; }
		const auto& proximitySound() const { return m_proximitySound; }
		const auto& lightColor() const { return m_lightColor; }
		const auto& lightIntensity() const { return m_lightIntensity; }
		const auto& lightOffset() const { return m_lightOffset; }
		const auto& lightAboveAll() const { return m_lightAboveAll; }
		const auto& lightOnGround() const { return m_lightOnGround; }
		const auto& lightScale() const { return m_lightScale; }

		float promixitySoundDistFactor() const { return m_promixitySoundDistFactor; }

	private:
		sf::Vector2i m_hotspot;
		vector<SpritePSI> m_psi;
		string m_proximitySound;
		float m_promixitySoundDistFactor;
		bool m_dummyTexture;
		bool m_lightAboveAll;
		bool m_lightOnGround;
		sf::Color m_lightColor;
		int m_lightIntensity;
		sf::Vector2i m_lightOffset;
		float m_lightScale;
};

