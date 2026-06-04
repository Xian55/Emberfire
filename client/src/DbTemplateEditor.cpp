#include "stdafx.h"
#include "DbTemplateEditor.h"
#include "SpellTemplateEditor.h"
#include "NpcTemplateEditor.h"
#include "NpcTemplateEditor.h"
#include "QuestTemplateEditor.h"
#include "GossipEditor.h"
#include "GossipOptionEditor.h"
#include "TextLines.h"
#include "Application.h"
#include "MapEditor.h"
#include "ContentMgr.h"
#include "ScriptDbEditor.h"
#include "ItemTemplateEditor.h"
#include "GameObjTemplateEditor.h"
#include "LootDbEditor.h"
#include "VendorDbEditor.h"
#include "VendorRandDbEditor.h"

#include <assert.h>

DbTemplateEditor::DbTemplateEditor(RenderObjectHolder& owner, const int id) :
	RenderObjectHolder(&owner, id),
	m_stage(RoEmpty)
{
	m_textLines = make_shared<TextLines>(owner, id, FontId::FrizBold, Util::GeoBox2d(0, 0, 500, 1000), 18);
	m_textLines->setClickableLines(true);
	m_textLines->setLeading(50);
	m_textLines->setShadowOffset(1);
	
	m_textLines->addLine("spell_template");
	m_textLines->addLine("item_template");
	m_textLines->addLine("npc_template");
	m_textLines->addLine("gameobject_template");
	m_textLines->addLine("quest_template");
	m_textLines->addLine("gossip");
	m_textLines->addLine("gossip_option");
	m_textLines->addLine("scripts");
	m_textLines->addLine("loot");
	m_textLines->addLine("npc_vendor");
	m_textLines->addLine("npc_vendor_random");

	if (dynamic_cast<MapEditor*>(&owner) != nullptr)
		m_textLines->addLine("<Map Editor>");

	sContentMgr->loadDbTablesDev();
}

DbTemplateEditor::~DbTemplateEditor()
{

}

void DbTemplateEditor::input() /*final*/
{
	__super::input();
	m_textLines->attemptInput();

	string clickedLine = m_textLines->popPendingClickedLine();

	if (!clickedLine.empty())
	{
		sContentMgr->playSound(SfxId::AlertEntry);

		if (clickedLine == "<Map Editor>")
			m_wantsClose = true;
		else
			setStage(clickedLine);
	}
}

void DbTemplateEditor::render() /*final*/
{
	__super::render();
	m_textLines->getTopLeftCornerRef().x = sApplication->sW() - 210;
	m_textLines->getTopLeftCornerRef().y = 40;
	m_textLines->attemptRender();
}
		
void DbTemplateEditor::setToEntry(const int entry)
{
	if (auto ptr = dynamic_pointer_cast<TableTemplateEditor>(getRenderObject(m_stage)))
		ptr->setEntry(entry);
}

bool DbTemplateEditor::setStage(const string& tableName, const bool overrideDuplicate /*= false*/)
{
	Ro stage;

	if (tableName == "spell_template")
		stage = DbTemplateEditor::Ro::RoSpellTemplateEditor;
	else if (tableName == "npc_template")
		stage = DbTemplateEditor::Ro::RoNpcTemplateEditor;
	else if (tableName == "gameobject_template")
		stage = DbTemplateEditor::Ro::RoGameObjTemplateEditor;
	else if (tableName == "quest_template")
		stage = DbTemplateEditor::Ro::RoQuestTemplateEditor;
	else if (tableName == "gossip")
		stage = DbTemplateEditor::Ro::RoGossipEditor;
	else if (tableName == "gossip_option")
		stage = DbTemplateEditor::Ro::RoGossipOptionEditor;
	else if (tableName == "scripts")
		stage = DbTemplateEditor::Ro::RoScriptDbEditor;
	else if (tableName == "item_template")
		stage = DbTemplateEditor::Ro::RoItemTemplateEditor;
	else if (tableName == "loot")
		stage = DbTemplateEditor::Ro::RoLootDbEditor;
	else if (tableName == "npc_vendor")
		stage = DbTemplateEditor::Ro::RoVendorDbEditor;
	else if (tableName == "npc_vendor_random")
		stage = DbTemplateEditor::Ro::RoVendorRandDbEditor;
	else
		return false;

	m_textLines->setSelectedLine(tableName);
	return setStage(stage, overrideDuplicate);
}

bool DbTemplateEditor::setStage(const Ro stage, const bool overrideDuplicate /*= false*/)
{
	if (m_stage == stage && !overrideDuplicate)
		return false;

	destroyObjectById(m_stage);
	m_stage = stage;

	if (m_stage == RoSpellTemplateEditor)
		addRenderObject(make_shared<SpellTemplateEditor>(*this, m_stage));
	else if (stage == RoNpcTemplateEditor)
		addRenderObject(make_shared<NpcTemplateEditor>(*this, m_stage));
	else if (stage == RoQuestTemplateEditor)
		addRenderObject(make_shared<QuestTemplateEditor>(*this, m_stage));
	else if (stage == RoGossipEditor)
		addRenderObject(make_shared<GossipEditor>(*this, m_stage));
	else if (stage == RoGossipOptionEditor)
		addRenderObject(make_shared<GossipOptionEditor>(*this, m_stage));
	else if (stage == RoScriptDbEditor)
		addRenderObject(make_shared<ScriptDbEditor>(*this, m_stage));
	else if (stage == RoItemTemplateEditor)
		addRenderObject(make_shared<ItemTemplateEditor>(*this, m_stage));
	else if (stage == RoGameObjTemplateEditor)
		addRenderObject(make_shared<GameObjTemplateEditor>(*this, m_stage));
	else if (stage == RoLootDbEditor)
		addRenderObject(make_shared<LootDbEditor>(*this, m_stage));
	else if (stage == RoVendorDbEditor)
		addRenderObject(make_shared<VendorDbEditor>(*this, m_stage));
	else if (stage == RoVendorRandDbEditor)
		addRenderObject(make_shared<VendorRandDbEditor>(*this, m_stage));
	else
		return false;

	return true;
}