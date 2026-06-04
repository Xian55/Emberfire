#include "stdafx.h"
#include "ScriptDbEditor.h"
#include "ScriptDbEntryEditor.h"

#include "..\Shared\Config.h"

ScriptDbEditor::ScriptDbEditor(RenderObjectHolder& owner, const int id) :
	MultiTableEditor(owner, id)
{
	setEntry(sConfig->getInt(getConfigName().c_str(), "Entry"));
}

ScriptDbEditor::~ScriptDbEditor()
{

}

string ScriptDbEditor::getTableName() const
{
	return "scripts";
}
		
string ScriptDbEditor::getKeyName() const
{
	return "scriptId";
}
		
string ScriptDbEditor::getConfigName() const 
{ 
	return "ScriptDbEditor";
}
		
string ScriptDbEditor::getOrderKey() const
{
	return "delay";
}

shared_ptr<TableTemplateEditor> ScriptDbEditor::spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary)
{
	auto ptr = make_shared<ScriptDbEntryEditor>(*this, Interface::EntriesOffset + entry);

	if (dictionary != nullptr)
		ptr->copyDictionary(*dictionary);
	else
		ptr->initDictionary();

	ptr->getTopLeftCornerRef().x = 100;
	ptr->getTopLeftCornerRef().y = 120 + (195 * idx);
	ptr->setEntry(entry);
	return ptr;
}