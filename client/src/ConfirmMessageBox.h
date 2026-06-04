#pragma once

#include "RenderObject.h"

class Button;
class TextBox;
class Sprite;

class ConfirmMessageBox : public RenderObject
{
	public:
		enum ConfirmBoxType
		{
			ConfirmBox_Ok,
			ConfirmBox_YesNo
		};

		enum ConfirmBoxResult
		{
			ConfirmBox_Waiting,

			// Yes == Ok
			ConfirmBox_Yes,
			ConfirmBox_No
		};

		enum ConfirmBoxCodes
		{
			ConfirmCode_Null,
			ConfirmCode_TerrainScript,
			ConfirmCode_SaveMapEditor,
			ConfirmCode_ExitGame,
			ConfirmCode_SelectCharacter,
			ConfirmCode_Disconnect,
			ConfirmCode_AbandonQuest,
			ConfirmCode_GkickSomeone,
			ConfirmCode_PromoteSomeone,
			ConfirmCode_DemoteSomeone,
			ConfirmCode_GuildInvite,
			ConfirmCode_MapeditorDeleteNpc,
			ConfirmCode_Respawn,
			ConfirmCode_SpellEditorNewKit,
			ConfirmCode_DbEditorEntry,
			ConfirmCode_BuyItem,
			ConfirmCode_BuyItemStack,
			ConfirmCode_DuelOffer,
			ConfirmCode_RepairConfirm,
			ConfirmCode_ActivateWaypoint,
			ConfirmCode_PartyOffer,
			ConfirmCode_ResetDungeons,
			ConfirmCode_RespecPrompt,
			ConfirmCode_DeleteCharacter,
			ConfirmCode_ArenaOffer,
			ConfirmCode_RetryLogin,

			ConfirmCode_Max,
		};

	public:
		ConfirmMessageBox(RenderObject& owner, const int id, const int code, const string& msg, const ConfirmBoxType type);
		virtual ~ConfirmMessageBox();

		ConfirmBoxCodes getCode() const { return m_code; }
		ConfirmBoxResult getResult() const { return m_result; }

		static int uniqueCode() { static int code = ConfirmCode_Max + 1; return ++code; }

	private:
		void input() final;
		void render() final;

		const ConfirmBoxCodes m_code;
		const string m_msg;
		const ConfirmBoxType m_type;

		unique_ptr<TextBox> m_text;
		shared_ptr<Sprite> m_spriteBackdrop;

		unique_ptr<Button> m_acceptButton;
		unique_ptr<Button> m_declineButton;

		ConfirmBoxResult m_result;
};

