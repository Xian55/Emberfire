#include "stdafx.h"
#include "GossipOptionEditor.h"
#include "GossipOptionEntryEditor.h"

#include "..\Shared\Config.h"

GossipOptionEditor::GossipOptionEditor(RenderObjectHolder& owner, const int id) :
	MultiTableEditor(owner, id)
{
	setEntry(sConfig->getInt(getConfigName().c_str(), "Entry"));
}

GossipOptionEditor::~GossipOptionEditor()
{

}

string GossipOptionEditor::getTableName() const
{
	return "gossip_option";
}
		
string GossipOptionEditor::getKeyName() const
{
	return "gossipId";
}
		
string GossipOptionEditor::getConfigName() const 
{
	return "GossipOptionEditor";
}
		
shared_ptr<TableTemplateEditor> GossipOptionEditor::spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary)
{
	auto ptr = make_shared<GossipOptionEntryEditor>(*this, Interface::EntriesOffset + entry);

	if (dictionary != nullptr)
		ptr->copyDictionary(*dictionary);
	else
		ptr->initDictionary();

	ptr->getTopLeftCornerRef().x = 100;
	ptr->getTopLeftCornerRef().y = 120 + (325 * idx);
	ptr->setEntry(entry);
	return ptr;
}