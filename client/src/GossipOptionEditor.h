#pragma once

#include "MultiTableEditor.h"

class ClientMap;
class GossipOptionEntryEditor;
class TableTemplateEditor;

class GossipOptionEditor : public MultiTableEditor
{
	enum Interface
	{
		EntriesOffset = 12500
	};

	public:
		GossipOptionEditor(RenderObjectHolder& owner, const int id);
		virtual ~GossipOptionEditor();

	protected:
		string getTableName() const final;
		string getKeyName() const final;		
		string getConfigName() const final;

		shared_ptr<TableTemplateEditor> spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary) final;
};