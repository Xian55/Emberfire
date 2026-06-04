#pragma once

#include "ClientObject.h"

#include "..\Shared\GameObjDefines.h"

class Sprite;
class SpriteAnimation;
class ClientObjectOverhead;
class Tooltip;

// Chests, doors and levers
class ClientGameObj : public ClientObject
{
	public:
		ClientGameObj(const int guid, const int entry, ClientMap* clientMap);
		virtual ~ClientGameObj();
		
		void render() final;
		void update() final;

		void playToggleSound(const GoDefines::ToggleState newState);
		void setToggleState(const GoDefines::ToggleState state);
		void addFlag(const GoDefines::Flags flag);

		bool isRenderedOnLayer(const ClientMap::Defines layer) final;

		int getEntry() const final { return m_entry; }

		GoDefines::Type getGoType() const { return m_goType; }

	protected:
		void notifyVariableChange(const ObjDefines::Variable var, const int value) final;
		void notifySubnameChanged() final;
		void notifyNameChanged() final;
		void setCurrentCell(const int c, MapCellClient& cell) final;

		void useTooltip();

		bool interactable() const;

		int m_entry;
		
		GoDefines::Type m_goType;

		shared_ptr<Tooltip> m_tooltip;
		shared_ptr<Sprite> m_model;
		shared_ptr<SpriteAnimation> m_spriteAnimation;
		shared_ptr<ClientObjectOverhead> m_overheads;

		sf::Vector2i m_sprAnimOffset;
};

