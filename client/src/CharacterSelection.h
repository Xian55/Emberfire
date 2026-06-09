#pragma once

#include "RenderObjectHolder.h"
#include "..\Shared\PlayerDefines.h"

class Sprite;
class TextBoxRo;
class Text;

class CharacterSelection :	public RenderObjectHolder
{
	public:
		enum Interface
		{
			LeftBtn,
			RightBtn,

			CreateCharacter,
			
			CharacterSlot1,
			CharacterSlot1_Name,
			CharacterSlot1_Subtext,
			CharacterSlot1_Portrait,

			CharacterSlot2,
			CharacterSlot2_Name,
			CharacterSlot2_Subtext,
			CharacterSlot2_Portrait,

			CharacterSlot3,
			CharacterSlot3_Name,
			CharacterSlot3_Subtext,
			CharacterSlot3_Portrait,
		};

		struct ButtonData
		{
			Interface button;
			Interface name;
			Interface subtext;
			Interface portrait;
		};

		struct Character
		{
			string name;
			PlayerDefines::Classes classId;
			int level;
			int guid;
			int portrait;
			int gender;
		};

	public:
		CharacterSelection(RenderObjectHolder& owner, const int id);
		virtual ~CharacterSelection();

		void registerCharacter(const string& name, const PlayerDefines::Classes classId, const int level, const int guid, const int portrait, const int gender);
		void clearCharacters();

		// ---- Lua view (the C++ screen is force-hidden; input/render never run): list reads + the direct
		//      enter/delete flows (the Lua screen does its own confirm dialogs). ----
		int  characterCount() const { return static_cast<int>(m_characters.size()); }
		bool characterAt(const int idx, Character& out) const;
		void enterCharacter(const int idx);    // GP_Client_EnterWorld + the s_pendingSelf stash + Loading
		void deleteCharacter(const int idx);   // GP_Client_DeleteCharacter + Loading

	protected:
		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;

	private:
		void input() final;
		void render() final;

		bool m_autoEntered = false;   // [Debug] AutoLogin dev auto-enter

		void updatePageTxt();
		void updateButtons();
		void sendEnterWorld(const Character& c);
		void maybeAutoEnter();
		void setButton(const ButtonData& buttondata, const Character& chardata);
		void initButton(const ButtonData& buttondata, const sf::Vector2i& offset);
		void pickCharacter(const int slot);
		void spawnCtxMenuForCharacter(const int slot);

		shared_ptr<Sprite> preparePortrait(shared_ptr<Sprite> ptr);

		int m_scrollOffset;

		Character m_chosenCharacter;

		vector<Character> m_characters;

		shared_ptr<Text> m_pageTxt;

		shared_ptr<Sprite> m_background;
		shared_ptr<Sprite> m_backgroundGui;

		shared_ptr<Button> m_leftBtn;
		shared_ptr<Button> m_rightBtn;
};

