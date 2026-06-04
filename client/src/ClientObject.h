#pragma once

#include "MouseableNode.h"
#include "WorldRenderable.h"

#include "..\Shared\MutualObject.h"

class World;
class AuraAnimation;

// Parent of all objects in a world map
class ClientObject : protected MouseableNode, public WorldRenderable, public MutualObject
{
	public:
		virtual ~ClientObject();	
		virtual void render() override;
		virtual void update() override;

		virtual int getEntry() const override { return 0; }

		virtual bool isMousedOver(const bool useRealPosition = false) override;
		virtual bool isLocal() const { return false; }

		virtual sf::Color getNameColor() const { return sf::Color::White; }
		
	public:
		void clearAuras();
		void auraAnimationIncrement(const int spellId);
		void auraAnimationDecrement(const int spellId);
		void setRespectAppMouseover(const bool b) { m_respectApplicationMouseover = b; }
		void setAlphaPct(const float pct) { m_alphaPct = pct; }
		void setFadeInPct(const float pct) { m_fadeInPct = pct; }
		
		bool isWithinScreenPosition() const;
		bool hasAura(const int spellId) const;

		float getAlphaPct() const { return m_alphaPct; }
		float getFadeInPct() const { return m_fadeInPct; }

		auto mouseableHeight() const { return m_height; }
		
	protected:
		ClientObject(const int guid, ClientMap* clientMap);
		
		virtual void notifyVariableChange(const ObjDefines::Variable var, const int value) override;

		void initHeight();
		void updateAuraAnimations(map<int, shared_ptr<AuraAnimation>>& m);
		void fillAuraAnimation(const int spellId, const int kit, map<int, shared_ptr<AuraAnimation>>& m);
		void drawAuras_below();
		void drawAuras_top();
		void setHeight(const int val) { m_height = val; }

		bool popClickedOnInWorld(const sf::Mouse::Button key);

		virtual bool mousedInWorld();

	private:
		int m_height{50};

		float m_alphaPct{1.f};
		float m_fadeInPct{1.f};

		bool m_respectApplicationMouseover;

		map<int, int> m_auras;

		// Separate container from m_auras because after an aura is removed, we want to let its animations finish up
		map<int, shared_ptr<AuraAnimation>> m_auraAnimationsOnTop;
		map<int, shared_ptr<AuraAnimation>> m_auraAnimationsBelow;
};

