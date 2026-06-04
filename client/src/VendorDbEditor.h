#pragma once

#include "MultiTableEditor.h"

class ClientMap;
class VendorDbEntryEditor;
class TableTemplateEditor;

class VendorDbEditor : public MultiTableEditor
{
	enum Interface
	{
		EntriesOffset = 12500
	};

	public:
		VendorDbEditor(RenderObjectHolder& owner, const int id);
		virtual ~VendorDbEditor();

	protected:
		string getTableName() const final;
		string getKeyName() const final;		
		string getConfigName() const final;
		string getOrderKey() const final;

		shared_ptr<TableTemplateEditor> spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary) final;
};