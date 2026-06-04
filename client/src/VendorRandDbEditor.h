#pragma once

#include "MultiTableEditor.h"

class ClientMap;
class VendorRandDbEntryEditor;
class TableTemplateEditor;

class VendorRandDbEditor : public MultiTableEditor
{
	enum Interface
	{
		EntriesOffset = 12500
	};

	public:
		VendorRandDbEditor(RenderObjectHolder& owner, const int id);
		virtual ~VendorRandDbEditor();

	protected:
		string getTableName() const final;
		string getKeyName() const final;		
		string getConfigName() const final;
		string getOrderKey() const final;

		shared_ptr<TableTemplateEditor> spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary) final;
};