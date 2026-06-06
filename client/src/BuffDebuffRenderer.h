#pragma once

#include "RenderObjectHolder.h"

#include "..\Shared\GamePacketServer.h"

class ClientUnit;
class World;

class BuffDebuffRenderer : public RenderObjectHolder
{
	enum Interface
	{
		BuffStart = 9500,
		BuffEnd = BuffStart + 8,
		DebuffStart = BuffEnd + 1,
		DebuffEnd = DebuffStart + 8
	};

	protected:
		enum Direction
		{
			LeftRight,
			RightLeft,
		};

	public:
		BuffDebuffRenderer(World& world, RenderObject* owner, const int id = 0);
		virtual ~BuffDebuffRenderer();

		// entry, duration
		void setBuffs(const vector<GP_Server_UnitAuras::AuraInfo>& buffs);
		void setDebuffs(const vector<GP_Server_UnitAuras::AuraInfo>& debuffs);

		// Stop rendering this object's aura icons (the Lua HUD replaces them). Clears any showing icons.
		void setSuppressed(const bool v);

	protected:
		void registerAuraObjs(sf::Vector2i* pAnchor);
		void processRightClickedId(const int id);
		void setUnit(shared_ptr<ClientUnit> unit);
		void refreshDispelTypeLogic();
		void setDirection(const Direction dir) { m_direction = dir; }

		sf::Vector2i m_auraAnchor;
		sf::Vector2i m_auraSpacing;		
		sf::Vector2i m_aurasOffset;
		string m_auraButtonFrame;

	private:
		void setAuras(const vector<GP_Server_UnitAuras::AuraInfo>& auras, const int interfaceStart, const int interfaceEnd, const bool renderDispelType);
		void createBuffIcon(const Interface id, sf::Vector2i* pAnchor);
		void createDebuffIcon(const Interface id, sf::Vector2i* pAnchor);

		bool highlightBuffDispelType() const;

		map<Interface, int> m_auraCasterGuids;
		bool m_suppressed{false};
		Direction m_direction{ LeftRight };
		shared_ptr<ClientUnit> m_unit;
		World& m_world;
};

