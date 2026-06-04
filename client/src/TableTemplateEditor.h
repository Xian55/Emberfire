#pragma once

#include "RenderObjectHolder.h"

class TextBoxRo;
class PromptBox;
class Button;

class TableTemplateEditor : public RenderObjectHolder
{
	struct FieldDisplay
	{
		shared_ptr<TextBoxRo> label;
		shared_ptr<PromptBox> prompt;
	};

	protected:
		enum Interface
		{
			DeleteEntryButton = 1,
			//
			LabelsBegin = 100,
			//

			//
			PromptBoxBegin = 1000,

			// Must be last
			FieldCtxMenu = 10000
		};	

	public:
		TableTemplateEditor(RenderObjectHolder& owner, const int id);
		virtual ~TableTemplateEditor();

		int getEntry() const { return m_entry; }

		virtual void copyDictionary(TableTemplateEditor const& cpy);
		virtual void deleteEntry();
		virtual void setEntry(const int spellId) = 0;

		virtual string fieldTable(const int field) const = 0;

	protected:
		virtual void input() override;
		virtual void notifyCtxMenuClicked(const int id, const string& lineClicked) override;

		virtual void initDictionary();
		virtual void setFieldValue(const int field, string str);

		virtual bool resetCurrentPrompt();
		virtual bool isMask(const int field) const;
		virtual bool fieldVisible(const int field) const;

		virtual int fieldEntry(const int field) const = 0;

		virtual string fieldName(const int field) const = 0;
		virtual string fieldValue(const int field) const;
		virtual string fieldKeyName(const int field) const = 0;
		virtual string fieldDbName(const int field) const = 0;
		virtual string fieldDescription(const int field) const;

		virtual string fieldValueToHumandReadable(const int field, string readable) const;
		virtual string huamnReadableToFieldValue(const int field, const string& readable) const;
				
		virtual vector<pair<string, sf::Color>> getFieldCtxOptions(const int field) const;

		virtual sf::Color getLabelColor(const int field) const;

		virtual void trimHumanReadableEntry(string& content, const int field) const;

		void allowDelete();

		shared_ptr<ConfirmMessageBox> processConfirmBox();

		map<string, string> m_strInterruptCauses;
		map<string, string> m_strDispelType;
		map<string, string> m_strEffects;
		map<string, string> m_strAttributes;
		map<string, string> m_strAuraType;
		map<string, string> m_strMechanics;
		map<string, string> m_strTargetType;
		map<string, string> m_strMagicSchool;
		map<string, string> m_strEntries;
		map<string, string> m_strStats;
		map<string, string> m_strEquipSlots;
		map<string, string> m_strUnitAnims;
		map<string, string> m_strBlendModes;
		map<string, string> m_strColors;
		map<string, string> m_strHitResults;
		map<string, string> m_strAbilitiesTabs;
		map<string, string> m_strFactions;
		map<string, string> m_strTypes;
		map<string, string> m_strAiTypes;
		map<string, string> m_strMovementTypes;
		map<string, string> m_strNpcFlags;
		map<string, string> m_strGameFlags;
		map<string, string> m_strConditions;
		map<string, string> m_strBoolean;
		map<string, string> m_strNpcModels;
		map<string, string> m_strQuestFlags;

		set<int> m_ignoreEnumForReadable;
		set<int> m_maskedValues;
		set<int> m_kitValues;
		set<int> m_longTxtValues;
		set<int> m_entryFormattedTxt;

		map<int, string> m_fieldDescriptions;
		map<int, map<string, string> const*> m_fieldEnumOptions;

		shared_ptr<Button> m_delButton;

		struct ConditionalLabel
		{
			struct NewLabelData
			{
				int field;
				string value;
				map<string, string> const* dictionary;
				bool mask;
			};
			
			vector<pair<int, int>> condition;
			vector<NewLabelData> newData;
		};

		void* m_chosenPrompt{ nullptr };
		
		int m_entry{0};
		int m_kitEntry{0};
		int m_insertEntry{0};
		int m_newKitField{0};
		int m_deleteConfirmCode{0};

		// Must be init by child
		int m_numFields{-1};
		int m_entryFieldId{-1};

		string m_insertEntryQuery;
		string m_insertTableName;

		vector<ConditionalLabel> m_conditionalLabels;

};

