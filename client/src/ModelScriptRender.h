#pragma once

#include "ModelScript.h"

class ModelScriptRender
{
	public:
		ModelScriptRender();
		~ModelScriptRender();

		void reset();
		void setModelScript(unique_ptr<ModelScript> ptr);
		void sync(ModelScriptRender& other);
		void setToLastFrame(const UnitDefines::AnimId animId);

		void setColor(const sf::Color c) { m_color = c; }
		void setBrightnessPct(const float a) { m_brightenPct = a; }
		void setColorOverlay(const sf::Color a) { m_colorOverlay = a; }
		void setOverlayBlendMode(const sf::BlendMode a) { m_overlayBlend = a; }
		void setSpeed(const float pct) { m_speed = pct; }
		void setTimerRatio(const float pct) { m_timerRatio = pct; }
		void setFreezeFrame(const int frameId) { m_freezeFrame = frameId; }
		void setScale(const float v) { m_scale = v; }

		sf::Color defaultOverlayColor() const { return sf::Color::White; }

		const sf::Color& getColor() const { return m_color; }

		bool done(const UnitDefines::AnimId animId) const;
		bool isAnimationPlayOnce(const UnitDefines::AnimId animId) const;

		auto getCurrentFrame() const { return m_currentFrame; }
		auto getNumFrames(const UnitDefines::AnimId animId) const;

		ModelScript* getScript() { return m_script.get(); }

		// Returns where the sprite was just rendered
		sf::FloatRect render(const sf::Vector2i& pos, const UnitDefines::AnimId animId, const UnitDefines::Direction dir, const bool forceLoop = false);

	private:
		void queueTextureLoad();
		bool forwardThenBack(const UnitDefines::AnimId animId) const;
		
		sf::Color m_colorOverlay;

		float m_speed{0};
		float m_brightenPct{0};
		float m_timerRatio{1.f};
		float m_scale{1.f};

		sf::Color m_color;
		sf::BlendMode m_overlayBlend;

		bool m_reverse{0};
		bool m_textureExists{0};

		int m_freezeFrame{-1};

		float m_timer{0};
		float m_timerTotal{0};

		size_t m_currentFrame{0};
		clock_t m_lastQueueTime{0};

		unique_ptr<ModelScript> m_script;
		shared_ptr<sf::Texture> m_texture;
};

