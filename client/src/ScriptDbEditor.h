#pragma once

#include "MultiTableEditor.h"

class ClientMap;
class ScriptDbEntryEditor;
class TableTemplateEditor;

class ScriptDbEditor : public MultiTableEditor
{
	enum Interface
	{
		EntriesOffset = 12500
	};

	public:
		ScriptDbEditor(RenderObjectHolder& owner, const int id);
		virtual ~ScriptDbEditor();

	protected:
		string getTableName() const final;
		string getKeyName() const final;		
		string getConfigName() const final;
		string getOrderKey() const final;

		shared_ptr<TableTemplateEditor> spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary) final;
};