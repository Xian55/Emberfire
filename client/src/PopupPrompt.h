#pragma once

#include "RenderObjectHolder.h"

class SpriteRo;
class Sprite;
class PromptBox;

class PopupPrompt : public RenderObjectHolder
{
	public:
		enum Interface
		{
			Background = 1,
			AcceptButton,
			DeclineButton,
			Title,
			Prompt,
		};

		enum Codes
		{
			SetGuildMotd = 1,
			MapeditorScaleObj,
			MapeditorSetZone,
			MapeditorSetArea,
			MapeditorWanderNpc,
			MapeditorRespawnNpc,
			MapeditorRespawnGo,
			TradeSetGold
		};

	public:
		PopupPrompt(RenderObject& owner, const int id, const string& titleStr, const Codes code, const bool numbersOnly = false, const bool allowZero = false);
		virtual ~PopupPrompt();

		auto getCode() const { return m_code; }
		auto isAccepted() const { return m_accepted; }

		const string& getContent() const;

	private:
		void input() final;
		void render() final;

		bool m_accepted;

		Codes m_code;
		
		shared_ptr<Sprite> m_bgSprite;
		shared_ptr<SpriteRo> m_bg;
		shared_ptr<PromptBox> m_promptBox;
};

