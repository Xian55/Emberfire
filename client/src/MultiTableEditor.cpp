#include "stdafx.h"
#include "MultiTableEditor.h"
#include "ContentMgr.h"
#include "TextBoxRo.h"
#include "PromptBox.h"
#include "Application.h"
#include "ConfirmMessageBox.h"
#include "Sprite.h"
#include "GameIcon.h"
#include "ClientMap.h"
#include "ClientNpc.h"
#include "SpellVisualKit.h"
#include "Abilities.h"
#include "GossipEntryEditor.h"

#include "..\Files.h"
#include "..\Vector.h"
#include "..\StringHelpers.h"

#include "..\Shared\Config.h"
#include "..\Shared\UnitDefines.h"
#include "..\Shared\Config.h"
#include "..\Shared\NpcDefines.h"

#include "..\SqlConnector\SqlConnector.h"
#include "..\SqlConnector\QueryResult.h"

MultiTableEditor::MultiTableEditor(RenderObjectHolder& owner, const int id) :
	TableTemplateEditor(owner, id)
{
	m_map = make_unique<ClientMap>(*this, 0);
	m_map->setRenderEmptyCells(true);
	m_map->setAllowCameraDrag(true);
	m_map->setShowClouds(false);
	m_map->resize(200);
	m_map->getCameraRef() = { 1480.f, 107.f };

	m_numFields = Field::NumFields;
	m_entryFieldId = Field::GTF_key;

	m_deleteConfirm.first = -1;
	m_newConfirmCode = -1;

	addRenderObject(make_shared<Button>(*this, "side_scroll_up", Interface::AddNewButton, sf::Vector2i{53, 34}));
	setMultiInput(true);
	initDictionary();
}

MultiTableEditor::~MultiTableEditor()
{

}

void MultiTableEditor::input() /*final*/
{
	m_map->attemptInput();
	__super::input();
	
	if (sApplication->getMouseWheelEvent().delta != 0)
	{
		m_scrollOffset -= int(sApplication->getMouseWheelEvent().delta);
		setEntry(m_entry);
	}
	if (auto confirmBox = sApplication->popConfirmBox({ m_deleteConfirm.first, m_newConfirmCode }))
	{
		if (confirmBox->getResult() == ConfirmMessageBox::ConfirmBox_Yes)
		{
			if (confirmBox->getCode() == m_deleteConfirm.first)
			{
				m_deleteConfirm.second->deleteEntry();
				setEntry(m_entry);
				return;
			}
			else if (confirmBox->getCode() == m_newConfirmCode)
			{
				shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
				db->query(Util::fmtStr("INSERT INTO `%s` ('%s') VALUES ('%d');", getTableName().c_str(), getKeyName().c_str(), m_entry));

				string err = db->error();

				if (!err.empty())
					sApplication->spawnPopup(err, ConfirmMessageBox::ConfirmBox_Ok, ConfirmMessageBox::ConfirmCode_Null);	

				setEntry(m_entry);
				return;
			}
		}
	}

	for (auto& itr : m_entries)
	{
		if (itr.second->popActivated())
		{
			int code = ConfirmMessageBox::uniqueCode();
			sApplication->spawnPopup(Util::fmtStr("Delete %d from %s?", itr.first->getEntry(), itr.first->fieldTable(m_entryFieldId).c_str()), ConfirmMessageBox::ConfirmBox_YesNo, code);
			m_deleteConfirm = { code, itr.first };
		}
	}

	if (popButtonId(Interface::AddNewButton))
	{
		if (m_entry != 0)
			sApplication->spawnPopup(Util::fmtStr("Add another entry to this %s?", getKeyName().c_str()), ConfirmMessageBox::ConfirmBox_YesNo, m_newConfirmCode = ConfirmMessageBox::uniqueCode());
		else
			sApplication->spawnPopup(Util::fmtStr("Enter a %s first.", getKeyName().c_str()), ConfirmMessageBox::ConfirmBox_Ok, 0);
	}
}

void MultiTableEditor::render() /*final*/
{
	m_map->attemptRender();
	__super::render();
}

void MultiTableEditor::notifyCtxMenuClicked(const int id, const string& lineClicked) /*final*/
{
	if (lineClicked.empty())
		return;

	if (id >= TableTemplateEditor::FieldCtxMenu)
	{
		Field field = Field(id - TableTemplateEditor::FieldCtxMenu);

		if (isMask(field))
		{
			uint64_t maskVal = _atoi64(fieldValue(field).c_str());
			const uint64_t newFlag = _atoi64(huamnReadableToFieldValue(field, lineClicked).c_str());

			if (maskVal & newFlag)
				maskVal &= ~newFlag;
			else
				maskVal |= newFlag;
		
			setFieldValue(field, to_string(maskVal));
		}
		else
		{
			setFieldValue(field, huamnReadableToFieldValue(field, lineClicked));
		}
	}
}

void MultiTableEditor::setFieldValue(const int field, string str) /*final*/
{
	setEntry(atoi(str.c_str()));
}

void MultiTableEditor::initDictionary() /*override*/
{
	__super::initDictionary();
}

string MultiTableEditor::fieldValue(const int field) const /*final*/
{
	return to_string(m_entry);
}

void MultiTableEditor::setEntry(const int spellId)
{
	m_entry = spellId;

	sConfig->setInt(getConfigName().c_str(), "Entry", m_entry);

	auto populate = [&](int start, int end, int xPos, int yPos, int width, int space)
	{
		for (int i = start, count = 0; i < end; ++i, ++count)
		{
			const int charactersize = 18;
			const int yVal = yPos + (count * 20);
			const auto field = Field(i);

			auto text = make_shared<TextBoxRo>(*this, TableTemplateEditor::LabelsBegin + i, FontId::FrizRegular, 400, charactersize);
			text->setString(fieldName(field), getLabelColor(field));
			text->updateTopLeftCorner(sf::Vector2i(xPos, yVal));
			addRenderObject(text);

			auto promptBox = make_shared<PromptBox>(*this, TableTemplateEditor::PromptBoxBegin + i, FontId::Arial, nullptr, sf::Vector2i(xPos + space, yVal), width, sf::Color::White);
			promptBox->setPromptCharacterSize(charactersize);
			promptBox->setMaxPromptString(1024);
			promptBox->setContent(fieldValueToHumandReadable(GTF_key, to_string(m_entry)));
			addRenderObject(promptBox);
		}
	};
	
	populate(0, Field::NumFields, 100, 40, 100, 190);
	
	for (auto& itr : m_entries)
	{
		destroyObjectById(itr.first->getId());
		destroyObjectById(itr.second->getId());
	}

	m_entries.clear();

	shared_ptr<SqlConnector> db = SqlConnector::create(SqlType::SQLite, sConfig->getString("GameDb", "Name"), sConfig->getString("GameDb", "Path"));
	
	if (auto result = db->query(Util::fmtStr("SELECT `entry` FROM `%s` WHERE `%s` = '%d' ORDER BY %s", getTableName().c_str(), getKeyName().c_str(), m_entry, getOrderKey().c_str())))
	{
		int i = 0;
		int count = 0;

		if (m_scrollOffset > int(result->fetchData().size()))
			m_scrollOffset = int(result->fetchData().size());

		if (m_scrollOffset < 0)
			m_scrollOffset = 0;

		for (auto& itr : result->fetchData())
		{
			++count;

			if (m_scrollOffset >= count)
				continue;

			int entry = itr[0].getInt32();
			auto ptr = spawnEntryObj(i++, entry, m_entries.empty() ? nullptr : m_entries[0].first.get());
			auto button = make_shared<Button>(*this, "cancel_back", Interface::EntriesButtonsOffset + entry);
			button->getTopLeftCornerRef().x = 35;
			button->getTopLeftCornerRef().y = ptr->getTopLeftCornerRef().y - 2;
			m_entries.push_back({ ptr, button });
			addRenderObject(ptr);
			addRenderObject(button);
		}
	}
}

string MultiTableEditor::getOrderKey() const
{
	return getKeyName();
}

string MultiTableEditor::fieldDbName(const int field) const
{
	return "unk";
}

int MultiTableEditor::fieldEntry(const int field) const
{	
	return m_entry;
}

string MultiTableEditor::fieldKeyName(const int field) const
{
	return fieldDbName(GTF_key);
}

string MultiTableEditor::fieldTable(const int field) const
{
	return "";
}

string MultiTableEditor::fieldName(const int field) const
{
	switch (field)
	{
		case GTF_key: return getKeyName();
	}									

	return "unk";
}

string MultiTableEditor::huamnReadableToFieldValue(const int field, const string& readable) const
{
	if (readable == "null")
		return "";
	
	return __super::huamnReadableToFieldValue(field, readable);
}

string MultiTableEditor::fieldValueToHumandReadable(const int field, string value) const
{
	if (value.empty())
		value = "0";
	
	return __super::fieldValueToHumandReadable(field, value);
}