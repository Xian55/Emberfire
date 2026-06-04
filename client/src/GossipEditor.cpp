#include "stdafx.h"
#include "GossipEditor.h"
#include "GossipEntryEditor.h"

#include "..\Shared\Config.h"

GossipEditor::GossipEditor(RenderObjectHolder& owner, const int id) :
	MultiTableEditor(owner, id)
{
	setEntry(sConfig->getInt(getConfigName().c_str(), "Entry"));
}

GossipEditor::~GossipEditor()
{

}

string GossipEditor::getTableName() const
{
	return "gossip";
}
		
string GossipEditor::getKeyName() const
{
	return "gossipId";
}
		
string GossipEditor::getConfigName() const 
{ 
	return "GossipEditor";
}
		
shared_ptr<TableTemplateEditor> GossipEditor::spawnEntryObj(const int idx, const int entry, TableTemplateEditor const* dictionary)
{
	auto ptr = make_shared<GossipEntryEditor>(*this, Interface::EntriesOffset + entry);

	if (dictionary != nullptr)
		ptr->copyDictionary(*dictionary);
	else
		ptr->initDictionary();

	ptr->getTopLeftCornerRef().x = 100;
	ptr->getTopLeftCornerRef().y = 120 + (265 * idx);
	ptr->setEntry(entry);
	return ptr;
}