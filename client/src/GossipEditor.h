#pragma once

#include "MultiTableEditor.h"

class ClientMap;
class GossipEntryEditor;
class TableTemplateEditor;

class GossipEditor : public MultiTableEditor
{
	enum Interface
	{
		EntriesOffset = 12500
	};

	public:
		GossipEditor(RenderObjectHolder& owner, const int id);
		virtual ~GossipEditor();

	protected:
		string getTableName() const final;
		string getKeyName() const final;		
		string getConfigName() const final;

		shared_ptr<TableTemplateEditor> spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary) final;
};