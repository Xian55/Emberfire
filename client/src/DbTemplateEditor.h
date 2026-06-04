#pragma once

#include "RenderObjectHolder.h"

class TextLines;

class DbTemplateEditor : public RenderObjectHolder
{
	public:
		enum Ro
		{
			RoEmpty,
			RoSpellTemplateEditor,
			RoNpcTemplateEditor,
			RoGameObjTemplateEditor,
			RoQuestTemplateEditor,
			RoGossipEditor,
			RoGossipOptionEditor,
			RoScriptDbEditor,
			RoLootDbEditor,
			RoVendorDbEditor,
			RoItemTemplateEditor,
			RoVendorRandDbEditor,
		};

	public:
		DbTemplateEditor(RenderObjectHolder& owner, const int id);
		virtual ~DbTemplateEditor();
		
		void setToEntry(const int entry);

		bool wantsClose() const { return m_wantsClose; }
		bool setStage(const string& tableName, const bool overrideDuplicate = false);

	private:
		void input() final;
		void render() final;

		bool setStage(const Ro stage, const bool overrideDuplicate = false);
		
		bool m_wantsClose{false};

		Ro m_stage;

		shared_ptr<TextLines> m_textLines;
};

