#pragma once

#include "RenderObject.h"

#include <vector>

class Button;
class ConfirmMessageBox;
class PopupPrompt;

class RenderObjectHolder : public RenderObject
{
	public:
		RenderObjectHolder(RenderObject* owner, const int id = 0);
		virtual ~RenderObjectHolder();

		void requestMoveToTop(const int id) { m_queueToBringToTop = id; }
		void raiseChild(const int id);   // bump child above all siblings (WoW Raise)
		void lowerChild(const int id);   // drop child below all siblings (WoW Lower)
		void sortByZLevel();             // stable-reorder children by getZLevel() (low=bottom, high=top)
		void setMultiInput(const bool v) { m_multiInput = v; }
		void addRenderObject(shared_ptr<RenderObject> ro);
		void setConfirmBoxDone(const int id);
		void toggleObjectHidden(const int id);
		void setPopupPromptDone(const int id);
		void registerContextMenu(const int id, const int childId, const sf::Vector2i pos, const vector<string>& lines);
		void registerContextMenuEx(const int id, const int childId, const sf::Vector2i pos, const vector<pair<string, sf::Color>>& lines);
		void unregisterCtxMenu(const int id, const int childId, const string& result);
		void setInputPrio(const int id);
		void clearInputPrio() { m_inputPrio = -1; }

		// Will be removed at startLoading of next intput()
		void destroyObjectById(const int id);
		void clear();
		void clearNow();

		int getMousedOverObj() const { return m_mousedOverObject; }

		bool isMousedOver(const bool useRealPosition = false) final;		
		bool isCtxOpen() const { return m_ctxMenuId != -1;}

		// Only applicable when m_multiInput is true
		bool allowedMouseOver(const int id) const;
		bool popButtonId(const int id) const;
		bool isMultiInput() const { return m_multiInput; }

		// Returns whether or not @pObj is the top input.
		virtual bool childMayInput(RenderObject const& obj) const override;
		
		int popFirstButtonId();
		int popFirstRightClickButtonId();

		shared_ptr<RenderObject> getFirstMousedOver(const bool useRealPosition = false, const int skipId = -1) const;
		shared_ptr<RenderObject> getRenderObject(const int id) const;
		shared_ptr<RenderObject> getTopInput() const;

		shared_ptr<RenderObject> popFirstButton();
		shared_ptr<RenderObject> popFirstRightClickButton();
		shared_ptr<RenderObject> popFirstLeftClickButton();
		shared_ptr<RenderObject> popKeyboundButton();
		
		// If you supply a vector of codes then you'll only pop it if the box matched one of those codes
		shared_ptr<PopupPrompt> popPopupPrompt(const vector<int>& fishingFor);
		shared_ptr<ConfirmMessageBox> popConfirmBox(const vector<int>& fishingFor);

	protected:
		virtual void input() override;
		virtual void render() override;
		
		shared_ptr<RenderObject> attachObj(shared_ptr<RenderObject> gi, const sf::Vector2i& offset);
		shared_ptr<RenderObject> attachAddObj(shared_ptr<RenderObject> gi, const sf::Vector2i& offset);

		void registerSkipRenderingObjThisFrame(const int id) { m_idsToSkipRenderingThisFrame.insert(id); }

	private:
		void removeObject(const int id);

		// Objects are rendered from startLoading to end with the end being on top.
		// This function will move the object with this id to the top, aka the end of m_renderObjects. 
		void moveObjectToTop(const int id);

		bool m_multiInput;

		int m_mousedOverObject;
		int m_queueToBringToTop;
		int m_currentPromptInput;
		int m_ctxMenuId;
		int m_inputPrio;

		set<int> m_idsToSkipRenderingThisFrame;

		shared_ptr<PopupPrompt> m_finishedPromptBox;
		shared_ptr<ConfirmMessageBox> m_finishedConfirmMessageBox;

		set<shared_ptr<RenderObject>> m_pendingDestroys;
		vector<shared_ptr<RenderObject>> m_renderObjects;
};

