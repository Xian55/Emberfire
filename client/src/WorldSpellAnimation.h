#pragma once

#include "WorldRenderable.h"

#include "..\Shared\UnitDefines.h"

class ClientObject;
class ParticleSystem;
class SpriteAnimation;
class SpellVisualKit;

class WorldSpellAnimation : public WorldRenderable
{
	public:
		WorldSpellAnimation(ClientMap* clientMap, const int spellId, const bool casting = false, const bool useGo = false);
		~WorldSpellAnimation();

		void update() final;
		void render() final;
		void setTimer(const int miliseconds);
		void setSource(shared_ptr<WorldRenderable> target);
		void setSpeed(const float s) { m_speed = s; }
		void setSoundPoint_Source(const sf::Vector2f& pos) { m_soundpoint_Source = pos; }
		void setSoundPoint_Target(const sf::Vector2f& pos) { m_soundpoint_Target = pos; }
		void setTarget(const Geo2d::Vector2& targetPos);
		void setTarget(shared_ptr<WorldRenderable> target);
		void cancel();

		bool isInit() const { return m_init; }

		virtual bool emitsLight(MapCellClient::LightSource* ls = nullptr) final;

		Geo2d::Vector2 deduceTargetPosition() const;

		static void preLoad(const int spellId, vector<shared_ptr<SpriteAnimation>>& output);

	private:	
		void initCamera() final;
		void initCasting();
		void initTravel();
		void initImpact();
		void init();
		
		void impact();
		void updateCasting();
		void updateTravel();

		bool finished() const;

		float getAngle(const float x, const float y) const;

		int m_spellId;
		int m_castingAnim;
		int m_animFreezeFrame_Casting{-1};

		clock_t m_castingExpiration;

		float m_speed;
		float m_orientation;
		float m_animSpeed_Casting{1.f};

		bool m_canceled;
		bool m_traveling;
		bool m_casting;
		bool m_init;
		bool m_go;

		sf::Vector2f m_soundpoint_Source{-1.f, -1.f};
		sf::Vector2f m_soundpoint_Target{-1.f, -1.f};
		sf::Vector2f m_lastCameraPosition;

		Geo2d::Vector2 m_targetPos;

		shared_ptr<WorldRenderable> m_targetObj;
		shared_ptr<WorldRenderable> m_sourceObj;
		
		unique_ptr<SpellVisualKit> m_travelingKit;
		unique_ptr<SpellVisualKit> m_castingKit;
		unique_ptr<SpellVisualKit> m_impactKit;
};

