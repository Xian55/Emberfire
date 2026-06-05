#pragma once

#include "BuffDebuffRenderer.h"

#include "..\Shared\GamePacketServer.h"

#include <vector>
#include <string>

class ClientUnit;
class Sprite;
class TextBox;
class Text;
class CastBar;
class ParticleSystem;
class RenderObjectHolder;

class UnitFrame : public BuffDebuffRenderer
{
	enum CtxMenuOptions
	{
		DdoWhisper,
		DdoInspect,
		DdoInvite,
		DdoTrade,
		DdoDuel,
		DdoCancel,
		DdoKick,
		DdoLeave,
		DdoResetDungeons,
		DdoPromote,
		DdoLeaveArenaQueue,
	};

	public:
		enum FrameStyle
		{
			NullStyle,
			LocalPlayer,
			MyTarget,
			PartyMember,
		};

	public:
		UnitFrame(RenderObject& owner, const int id, const sf::Vector2i pos);
		virtual ~UnitFrame();

		void setFrameStyle(const FrameStyle v);
		void setUnit(shared_ptr<ClientUnit> unit);
		void playMessage(const string& msg, sf::Color color);
		void setEnableRepairIcon(const bool v) { m_repairIcon = v; }
		void setOffline(const bool v) { m_offline = v; }

		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;
		// build+register the menu on `host` (default: owner). The Lua frames pass the Lua manager so it renders
		// above the Lua HUD, and may append extraLines (e.g. Lock/Unlock) handled Lua-side.
		void openContextMenu(RenderObjectHolder* host = nullptr, const std::vector<std::string>& extraLines = {});

		int getPortraitOffset() const { return m_portraitOffset; }

		shared_ptr<Sprite> getPortraitRef() const { return m_portrait; }

		ClientUnit* getUnitPtr() const { return m_unit.get(); }
		Sprite* getPortraitPtr() const { return m_portrait.get(); }
		
		const sf::Vector2i& getPortraitPosition() const { return m_portraitPos; }

		static void renderBar(Sprite& spr, const sf::Vector2i topLeftCorner, const float maxWidth, const float pct);
		static sf::IntRect renderBarCrop(Sprite& spr, const sf::Vector2i topLeftCorner, float pct);

	private:
		void input() final;
		void render() final;

		void decidePortrait();		
		
		bool setPortraitByName(string name);

		static string ctxMenuOptionStr(const CtxMenuOptions id);

		bool m_lastFriendlieness{false};
		bool m_repairIcon{false};
		bool m_hasBrokenIcon{false};
		bool m_inArenaQueue{false};
		bool m_offline{false};

		float m_hpPctRender{0};
		float m_manaPctRender{0};
		float m_msgTimer{0};

		int m_portraitRadius{0};
		int m_portraitOffset{0};

		FrameStyle m_frameStyle{FrameStyle::NullStyle};

		set<string> m_invalidPortraits;

		sf::Vector2i m_hpTopLeft;
		sf::Vector2i m_manaTopLeft;
		sf::Vector2i m_lvlTxtPos;
		sf::Vector2i m_portraitPos;
		sf::Vector2i m_hpPctPos;
		sf::Vector2i m_mpPctPos;
		sf::Vector2i m_namePos;
		sf::Vector2i m_elitePos;
		sf::Vector2i m_bossPos;
		sf::Vector2i m_combatPos;
		sf::Vector2i m_repairPos;
		sf::Vector2i m_leaderPos;
		sf::Vector2i m_combatMsgPos;
		sf::Vector2i m_hpTxtPos;
		sf::Vector2i m_mpTxtPos;

		shared_ptr<Sprite> m_frameSprite;
		shared_ptr<Sprite> m_hpSprite;
		shared_ptr<Sprite> m_manaSprite;
		shared_ptr<Sprite> m_portrait;
		shared_ptr<Sprite> m_levelFrameSprite;
		shared_ptr<Sprite> m_eliteSprite;
		shared_ptr<Sprite> m_bossSprite;		
		shared_ptr<Sprite> m_combatSprite;		
		shared_ptr<Sprite> m_criticalStatusSprite;
		shared_ptr<Sprite> m_partyLeaderSprite;
		shared_ptr<CastBar> m_castBar;

		shared_ptr<ClientUnit> m_unit;

		unique_ptr<TextBox> m_lvlTxt;

		unique_ptr<Text> m_hpPctTxt;
		unique_ptr<Text> m_manaPctTxt;
		unique_ptr<Text> m_hpTxt;
		unique_ptr<Text> m_manaTxt;
		unique_ptr<Text> m_nameTxt;
		unique_ptr<Text> m_combatMsgTxt;
		unique_ptr<ParticleSystem> m_criticalStatusPsi;
};

