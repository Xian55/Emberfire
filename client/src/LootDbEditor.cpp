#include "stdafx.h"
#include "LootDbEditor.h"
#include "LootDbEntryEditor.h"

#include "..\Shared\Config.h"

LootDbEditor::LootDbEditor(RenderObjectHolder& owner, const int id) :
	MultiTableEditor(owner, id)
{
	setEntry(sConfig->getInt(getConfigName().c_str(), "Entry"));
}

LootDbEditor::~LootDbEditor()
{

}

string LootDbEditor::getTableName() const
{
	return "loot";
}
		
string LootDbEditor::getKeyName() const
{
	return "lootId";
}
		
string LootDbEditor::getConfigName() const 
{ 
	return "LootDbEditor";
}
		
string LootDbEditor::getOrderKey() const
{
	return "entry";
}

shared_ptr<TableTemplateEditor> LootDbEditor::spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary)
{
	auto ptr = make_shared<LootDbEntryEditor>(*this, Interface::EntriesOffset + entry);

	if (dictionary != nullptr)
		ptr->copyDictionary(*dictionary);
	else
		ptr->initDictionary();

	ptr->getTopLeftCornerRef().x = 100;
	ptr->getTopLeftCornerRef().y = 120 + (250 * idx);
	ptr->setEntry(entry);
	return ptr;
}