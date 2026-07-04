#pragma once

#include "RenderObjectHolder.h"

class Sprite;
class SelectionButtons;
class Text;

class Options : public RenderObjectHolder
{
	public:
		enum Stage
		{
			Null,
			Main,
			System,
			Keybindings
		};

		enum Interface
		{
			Main_ButtonSystem = 1,
			Main_ButtonLogout,
			Main_ButtonExitGame,
			Main_ButtonReturnGame,

			System_Tabs,

			System_MusicVolumeBar,
			System_SfxVolumeBar,			
			System_CtmTick,
			System_StickyTick,
			System_EnemyNameplateTick,
			System_FriendlyNameplateTick,
			System_YourNameTick,
			System_PlayerRanksTick,
			System_ShowPlayerNameTick,
			System_ShowNpcNameTick,
			System_WindowMode,
			System_WindowModeCtx,
			System_Resolution,
			System_ResolutionCtx,
			System_UIScale,
			System_UIScaleCtx,

			Keybinds_List_1_A,
			Keybinds_List_1_B,
			Keybinds_List_2_A,
			Keybinds_List_2_B,

			Keybinds_Scrollbar_1,
			Keybinds_Scrollbar_2,

			ButtonGoBack,

			BackgroundImg
		};

	public:
		Options(RenderObjectHolder& owner, const int id);
		virtual ~Options();

		void setStage(const Stage stage);

	private:
		void input() final;
		void render() final;
		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;

		void setBackground(const string& filename);
		void save();
		void recalcOptionsBounds();

		Stage m_stage;

		int m_keybind_clicked_1_A;
		int m_keybind_clicked_1_B;
		int m_keybind_clicked_2_A;
		int m_keybind_clicked_2_B;

		
		unique_ptr<Text> m_windowModeTxt;
		unique_ptr<Text> m_resolutionTxt;
		unique_ptr<Text> m_uiScaleTxt;
		unique_ptr<Text> m_uiScaleCaption;

		shared_ptr<Sprite> m_bgSprite;
		shared_ptr<SelectionButtons> m_viewChoices;
};

