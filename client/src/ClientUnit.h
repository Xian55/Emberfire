#pragma once

#include "ClientObject.h"
#include "ModelScript.h"

#include "..\Shared\UnitDefines.h"
#include "..\Shared\MutualUnit.h"
#include "..\Shared\ChatDefines.h"
#include "..\Shared\SpellDefines.h"
#include "..\Shared\GamePacketServer.h"

class Sprite;
class ModelScriptRender;
class ClientObjectOverhead;
class World;
class WorldSpellAnimation;
class Tooltip;

// Players and NPC's
class ClientUnit : public ClientObject, public MutualUnit
{
	public:
		enum BodyPart
		{
			Feet,
			Legs,
			Torso,
			Hands,
			Head,
			Weapon,
			Offhand,
			WeaponRanged,
		};

	public:
		virtual ~ClientUnit();
		virtual void update() override;

		// MutualUnit's mana/stat helpers read through this hook; forward to the object var map.
		int getUnitVariable(const ObjDefines::Variable var) const override { return getVariable(var); }

	public:
		virtual void died() {}

		// Return false if the unit has so such sound types
		virtual bool playSoundAttack() { return false; }
		virtual bool playSoundAggro() { return false; }
		virtual bool playDeathSound() { return false; }
		virtual bool playSoundDamaged() { return false; }
		
		virtual sf::Color getNameColor() const override;
		virtual UnitDefines::Faction faction() const { return UnitDefines::Faction::Neutral; }
		
	public:
		void setIsLocal(const bool v) { m_isLocal = v; }
		void setNpcModel(const string& modelName);
		void setBodyPart(const string& modelName, const BodyPart bpart);
		void clearBodyPart(const BodyPart bpart);
		void playAnimation(const UnitDefines::AnimId id);
		void chatMsg(const string& str, const ChatDefines::Channels c);
		void playSoundMelee(ClientUnit& victim, const SpellDefines::HitResult hitResult);
		void setCastBar(shared_ptr<WorldSpellAnimation> sa, const int spellId, const int timer);
		void flashBrighten(const float pct = 0.2f);
		void setBuffs(const vector<GP_Server_UnitAuras::AuraInfo>& auras);
		void setDebuffs(const vector<GP_Server_UnitAuras::AuraInfo>& auras);
		void setServerPosition(const Geo2d::Vector2& pos) { m_serverLocation = pos; }
		void setServerSpline(const vector<Geo2d::Vector2>& s) { m_serverSpline = s; }
		void setAnimFreezeFrame(const int frameId) { m_animFreezeFrame = frameId; }
		void setAnimPlaybackSpeed(const float pct) { m_animPlaybackSpeed = pct; }
		void useTooltip();
		void expireTooltip();
		void finishAnimation();

		// The m_modelColor gets reset to White after being used for 1 frame
		void registerModelColor(const sf::Color& col) { m_modelColor = col; }
		
		auto getCastStartTime() const { return m_castStartTime; }
		auto getCastStopTime() const { return m_castStopTime; }
		auto getAnimation() const { return m_animationId; }

		const auto& getBuffs() const { return m_buffs; }
		const auto& getDebuffs() const { return m_debuffs; }
		const auto& getServerPosition() const { return m_serverLocation; }
		const auto& getOverhead() const { return m_overheads; }

		bool emitsLight(MapCellClient::LightSource* ls = nullptr) final;
		bool isLocal() const final { return m_isLocal; }
		
		int getCastSpellId() const { return m_castSpellId; }
		
	public:
		static UnitDefines::Direction computeDirection(float orientation);
		static sf::Color getFactionColor(const UnitDefines::Faction fact);

	protected:
		ClientUnit(const int guid, ClientMap* clientMap);

		void setSheath(const BodyPart bpart) { m_sheath = bpart; }
		void registerFootstepSounds(const vector<string> filenames) { m_footstepSounds = filenames; }
		float getCombatVolume() const;

	protected:
		virtual void render() override;
		virtual void notifyVariableChange(const ObjDefines::Variable var, const int value) override;
		virtual void setCurrentCell(const int c, MapCellClient& cell) override;

		virtual void playSoundMelee(ClientUnit& victim) {}

		virtual bool isMousedOver(const bool useRealPosition = false);

	private:
		void notifySubnameChanged() final;
		void notifyNameChanged() final;

		void doFootstepSounds();
		void setAnimation(const UnitDefines::AnimId id);
		void processBounds(sf::FloatRect& spr);
		void setModelScriptRenderVariables(ModelScriptRender& r) const;
		void parseAuraVector(const vector<GP_Server_UnitAuras::AuraInfo>& incomingAuras, vector<GP_Server_UnitAuras::AuraInfo>& myAuras);

		bool isAnimationFinished() const;
		bool isAnimationPlayOnce(const UnitDefines::AnimId id) const;
		bool isAnimationPlayableWhileMoving(const UnitDefines::AnimId id) const;

		ModelScriptRender const* fetchFirstModelScript() const;

		bool m_isLocal;
		bool m_previousAlive;
		bool m_showRanks{true};

		BodyPart m_sheath{BodyPart::Weapon};

		int m_castSpellId;
		int m_animFreezeFrame{-1};
		
		float m_brightenPct;
		float m_animPlaybackSpeed{1.f};
		float m_scale{1.f};

		clock_t m_castStartTime;
		clock_t m_castStopTime;
		clock_t m_nextFootstep;
		clock_t m_nextDrawTooltip = 0;

		sf::Color m_modelColor;
		uint8_t m_modelAlpha{255};
		Geo2d::Vector2 m_serverLocation;
		vector<Geo2d::Vector2> m_serverSpline;
		vector<Geo2d::Vector2> m_serverSlide;

		UnitDefines::AnimId m_animationId;

		shared_ptr<Sprite> m_portrait;
		shared_ptr<Sprite> m_selectedSprite;
		shared_ptr<WorldSpellAnimation> m_castAnimation;
		shared_ptr<ClientObjectOverhead> m_overheads;
		shared_ptr<Tooltip> m_tooltip;

		unique_ptr<ModelScriptRender> m_npcModel;
		
		map<BodyPart, unique_ptr<ModelScriptRender>> m_bodyParts;
		
		vector<string> m_footstepSounds;		
		vector<GP_Server_UnitAuras::AuraInfo> m_buffs;
		vector<GP_Server_UnitAuras::AuraInfo> m_debuffs;
};

