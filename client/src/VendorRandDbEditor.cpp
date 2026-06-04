#include "stdafx.h"
#include "VendorRandDbEditor.h"
#include "VendorRandDbEntryEditor.h"

#include "..\Shared\Config.h"

VendorRandDbEditor::VendorRandDbEditor(RenderObjectHolder& owner, const int id) :
	MultiTableEditor(owner, id)
{
	setEntry(sConfig->getInt(getConfigName().c_str(), "Entry"));
}

VendorRandDbEditor::~VendorRandDbEditor()
{

}

string VendorRandDbEditor::getTableName() const
{
	return "npc_vendor_random";
}
		
string VendorRandDbEditor::getKeyName() const
{
	return "npc_entry";
}
		
string VendorRandDbEditor::getConfigName() const 
{ 
	return "VendorRandDbEditor";
}
		
string VendorRandDbEditor::getOrderKey() const
{
	return "entry";
}

shared_ptr<TableTemplateEditor> VendorRandDbEditor::spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary)
{
	auto ptr = make_shared<VendorRandDbEntryEditor>(*this, Interface::EntriesOffset + entry);

	if (dictionary != nullptr)
		ptr->copyDictionary(*dictionary);
	else
		ptr->initDictionary();

	ptr->getTopLeftCornerRef().x = 100;
	ptr->getTopLeftCornerRef().y = 120 + (200 * idx);
	ptr->setEntry(entry);
	return ptr;
}