#pragma once

#include "..\Shared\UnitDefines.h"

class ParticleSystem;
class SpriteAnimation;
class ClientObject;

class SpellVisualKit
{
	public:
		enum BlendMode
		{
			BlendAlpha = -1,
			BlendAdd,
			BlendMultiply,
			BlendNone,
			BlendScreen,
		};

	// Obj is used for measurements
	public:
		SpellVisualKit(const int kitId, ClientObject const* obj);
		virtual ~SpellVisualKit();

		void initSound(const sf::Vector2f& screenPosition = sf::Vector2f{-1.f, -1.f});
		void stopPsi();
		void movePsiTo(const sf::Vector2f& pos, const bool moveParticles);
		void drawSprites(const sf::Vector2i& screenPosition, const float orientation);
		void transposePsi(const sf::Vector2f& transposePos);
		void drawPsi();
		void update();
		void stop();
		void deletePsi();
		void loop();
		void loopSound();
		void cancel();
		void stopSound();
		void stopLoopingSound();
		
		bool getUnitGlowData(sf::Color& outputColor);
		bool getGroundGlowData(sf::Color& outputColor);

		bool hasSprAnimsPlaying() const;
		bool waitForSprAnims() const;
		bool finished() const;

		static sf::BlendMode getSfBlend(const BlendMode myenums);

	private:
		void updateOffsets();

		float getGlowAlpha() const;

		int keywords(const string& val);

		int m_kitId;

		bool m_waitForSprAnims{false};

		struct SpAnim
		{
			shared_ptr<SpriteAnimation> anim;
			sf::Vector2i offset;
			BlendMode blendMode{BlendAlpha};
			bool topmost{false};
		};

		float m_ageSeconds;
		float m_duration;

		SpAnim m_sprAnim[2];
		map<UnitDefines::Direction, SpAnim> m_travelingAnimDir;

		ClientObject const* m_obj;
		unique_ptr<ParticleSystem> m_psi;
		sf::Vector2i m_psiOffset;
		
		int64_t m_groundGlowColor;
		int64_t m_unitGlowColor;

		string m_sound;

		shared_ptr<SfSoundSafe> m_soundsf;
};

