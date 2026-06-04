#include "stdafx.h"
#include "VendorDbEditor.h"
#include "VendorDbEntryEditor.h"

#include "..\Shared\Config.h"

VendorDbEditor::VendorDbEditor(RenderObjectHolder& owner, const int id) :
	MultiTableEditor(owner, id)
{
	setEntry(sConfig->getInt(getConfigName().c_str(), "Entry"));
}

VendorDbEditor::~VendorDbEditor()
{

}

string VendorDbEditor::getTableName() const
{
	return "npc_vendor";
}
		
string VendorDbEditor::getKeyName() const
{
	return "npc_entry";
}
		
string VendorDbEditor::getConfigName() const 
{ 
	return "VendorDbEditor";
}
		
string VendorDbEditor::getOrderKey() const
{
	return "item, entry";
}

shared_ptr<TableTemplateEditor> VendorDbEditor::spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary)
{
	auto ptr = make_shared<VendorDbEntryEditor>(*this, Interface::EntriesOffset + entry);

	if (dictionary != nullptr)
		ptr->copyDictionary(*dictionary);
	else
		ptr->initDictionary();

	ptr->getTopLeftCornerRef().x = 100;
	ptr->getTopLeftCornerRef().y = 120 + (75 * idx);
	ptr->setEntry(entry);
	return ptr;
}