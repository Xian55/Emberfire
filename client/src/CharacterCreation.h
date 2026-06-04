#pragma once

#include "RenderObjectHolder.h"

#include "..\Shared\PlayerDefines.h"

class Sprite;
class TextLines;
class PromptBox;
class SelectionButtons;
class SpriteRo;

class CharacterCreation : public RenderObjectHolder
{
	public:
		enum Interface
		{
			CreateCharacter = 1,
			NamePrompt,
			ClassSelection,
			PortraitsLeft,
			PortraitsRight,
			CancelBack,
			FemaleButton,
			MaleButton
		};

	public:
		CharacterCreation(RenderObjectHolder& owner, const int id);
		virtual ~CharacterCreation();

	private:
		void input() final;
		void render() final;

		void setSelectedClass(const PlayerDefines::Classes classId);
		void updatePortrait();

		int m_portairtId;
		int m_portraitOffset{0};

		PlayerDefines::Gender m_gender;

		shared_ptr<Sprite> m_background;
		shared_ptr<Sprite> m_backgroundGui;
		shared_ptr<Sprite> m_portraitSprite;
		shared_ptr<SpriteRo> m_selectedGender;

		shared_ptr<PromptBox> m_namePrompt;
		shared_ptr<SelectionButtons> m_classChoices;
};

