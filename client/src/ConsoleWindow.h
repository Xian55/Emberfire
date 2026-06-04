#pragma once

#include "ExpandableWindow.h"

#include "..\Shared\Commands.h"

class RenderObjectHolder;
class Button;
class PromptBox;

class ConsoleWindow : public ExpandableWindow, protected Commands
{
	enum ConsoleColors
	{		
		ErrorRed = 0xEF476BFF,
		WarningYellow = 0xFFD700FF
	};

	public:
		ConsoleWindow(RenderObject& owner, const int id);
		virtual ~ConsoleWindow();

		void performCommand(const string& text);
		void executeFile(const string& filename);

		void error(const string& txt) final;
		void warning(const string& txt) final;
		void print(const string& txt);

		void setHidden(const bool b) final;

	protected:
		void input() final;
		void render() final;

	private:
		static bool handleQuitCmd(const char* args, Commands* thisptr);
		static bool handleDbEditorToggle(const char* args, Commands* thisptr);
		static bool handleDbEditorStage(const char* args, Commands* thisptr);
		static bool handleMapEditorToggle(const char* args, Commands* thisptr);
		static bool handleMapEditorLoad(const char* args, Commands* thisptr);
		static bool handleMapEditorSave(const char* args, Commands* thisptr);
		static bool handleMapEditorSetName(const char* args, Commands* thisptr);
		static bool handleMapEditorNew(const char* args, Commands* thisptr);
		static bool handleMapEditorSpawnExampleNpc(const char* args, Commands* thisptr);
		static bool handleGameSetStage(const char* args, Commands* thisptr);
		static bool handleGameToggle(const char* args, Commands* thisptr);
		static bool handleMapEditorSpawnExampleSpellAnim(const char* args, Commands* thisptr);
		static bool handleReloadContentMgr(const char* args, Commands* thisptr);
		static bool handleDumpTextures(const char* args, Commands* thisptr);
		static bool handleDeltaCommand(const char* args, Commands* thisptr);
		static bool handleLuaExec(const char* args, Commands* thisptr);
		static bool handleLuaSelfTest(const char* args, Commands* thisptr);

	private:
		CCommand* getCommandTable();

		unique_ptr<DraggableNode> m_dragNode;
		shared_ptr<Button> m_closeButton;
		shared_ptr<PromptBox> m_promptBox;
};