#include "stdafx.h"
#include "RenderObjectHolder.h"
#include "Button.h"
#include "ConfirmMessageBox.h"
#include "PopupPrompt.h"
#include "GameIcon.h"
#include "ContextMenu.h"

#include <assert.h>

RenderObjectHolder::RenderObjectHolder(RenderObject* owner, const int id /*= 0*/) :
	RenderObject(owner, id),
	m_multiInput(false),
	m_mousedOverObject(0),
	m_queueToBringToTop(-1),
	m_inputPrio(-1),
	m_ctxMenuId(-1)
{

}

RenderObjectHolder::~RenderObjectHolder()
{

}

void RenderObjectHolder::input()
{
	// The object should no longer exist anywhere else and is now destroyed.
	m_pendingDestroys.clear();

	// Input everything vs. input top-only
	if (m_multiInput && m_ctxMenuId == -1)
	{
		m_mousedOverObject = 0;

		for (int i = static_cast<int>(m_renderObjects.size()) - 1; i >= 0; --i)
		{
			shared_ptr<RenderObject> obj = m_renderObjects[i];

			if (obj->isHidden())
				continue;

			if (obj->isMousedOver())
			{
				m_mousedOverObject = obj->getId();
				break;
			}
		}

		vector<shared_ptr<RenderObject>> cpy = m_renderObjects;

		for (int i = static_cast<int>(cpy.size()) - 1; i >= 0; --i)
			cpy[i]->attemptInput();
	}
	else
	{
		if (m_ctxMenuId != -1)
		{
			if (auto obj = getRenderObject(m_ctxMenuId))
				obj->attemptInput();
		}
		else if (m_inputPrio != -1)
		{
			if (auto obj = getRenderObject(m_inputPrio))
				obj->attemptInput();
		}
		else if (auto obj = getTopInput())
		{
			obj->attemptInput();
		}
	}
}

shared_ptr<RenderObject> RenderObjectHolder::attachObj(shared_ptr<RenderObject> gi, const sf::Vector2i& offset)
{
	gi->setAnchor(&getTopLeftCornerRef());
	gi->setOffset(offset);
	return gi;
}

shared_ptr<RenderObject> RenderObjectHolder::attachAddObj(shared_ptr<RenderObject> gi, const sf::Vector2i& offset)
{
	auto ptr = attachObj(gi, offset);
	addRenderObject(ptr);
	return ptr;
}

void RenderObjectHolder::render()
{
	if (m_queueToBringToTop != -1)
	{
		moveObjectToTop(m_queueToBringToTop);
		m_queueToBringToTop = -1;
	}

	// Update all of the render objects that we contain
	if (!m_renderObjects.empty())
	{
		vector<shared_ptr<RenderObject>> cpy = m_renderObjects;

		for (size_t i = 0; i < cpy.size(); ++i)
		{
			if (!m_idsToSkipRenderingThisFrame.empty() && m_idsToSkipRenderingThisFrame.find(cpy[i]->getId()) != m_idsToSkipRenderingThisFrame.end())
				continue;

			cpy[i]->attemptRender();
		}
	}

	m_idsToSkipRenderingThisFrame.clear();
}

void RenderObjectHolder::addRenderObject(shared_ptr<RenderObject> ro)
{
	// Value -1 is used as a key for certain logic in this class
	ASSERT(ro->getId() != -1);
	destroyObjectById(ro->getId());
	m_renderObjects.push_back(ro);
}

void RenderObjectHolder::removeObject(const int id)
{
	for (auto itr = m_renderObjects.begin(); itr != m_renderObjects.end(); ++itr)
	{
		if ((*itr)->getId() == id)
		{
			m_renderObjects.erase(itr);
			return;
		}
	}
}

void RenderObjectHolder::moveObjectToTop(const int id)
{
	// There needs to be 2 or more for this to make sense
	if (m_renderObjects.size() <= 1)
		return;

	// Moot
	if (m_renderObjects[m_renderObjects.size() - 1]->getId() == id)
		return;

	size_t position = 0;

	for (; position < m_renderObjects.size(); ++position)
	{
		if (m_renderObjects[position]->getId() == id)
			break;
	}

	if (position >= m_renderObjects.size())
		return;

	auto itr = (m_renderObjects.begin() + position);
	shared_ptr<RenderObject> obj = *itr;
	m_renderObjects.erase(itr);
	m_renderObjects.push_back(obj);
}

bool RenderObjectHolder::isMousedOver(const bool useRealPosition /*= false*/)
{
	for (auto& itr : m_renderObjects)
	{
		if (!itr->isHidden() && itr->isMousedOver())
			return true;
	}

	return __super::isMousedOver(useRealPosition);
}

int RenderObjectHolder::popFirstRightClickButtonId()
{
	if (auto b = popFirstRightClickButton())
		return b->getId();

	return -1;
}

int RenderObjectHolder::popFirstButtonId()
{
	if (auto b = popFirstButton())
		return b->getId();

	return -1;
}

shared_ptr<RenderObject> RenderObjectHolder::getFirstMousedOver(const bool useRealPosition /*= false*/, const int skipId /*= -1*/) const
{
	if (m_renderObjects.empty())
		return nullptr;
	
	for (int i = static_cast<int>(m_renderObjects.size()) - 1; i >= 0; --i)
	{
		shared_ptr<RenderObject> obj = m_renderObjects[i];

		if (skipId != -1 && obj->getId() == skipId)
			continue;

		if (!obj->isHidden() && obj->MouseableNode::isMousedOver(useRealPosition))
			return obj;
	}

	return nullptr;
}

shared_ptr<RenderObject> RenderObjectHolder::getRenderObject(const int id) const
{
	for (auto& itr : m_renderObjects)
	{
		if (itr->getId() == id)
			return itr;
	}

	return nullptr;
}

shared_ptr<RenderObject> RenderObjectHolder::getTopInput() const
{
	if (m_renderObjects.empty())
		return nullptr;

	if (m_renderObjects.size() == 1)
		return m_renderObjects[0];

	for (int i = static_cast<int>(m_renderObjects.size()) - 1; i >= 0; --i)
	{
		shared_ptr<RenderObject> obj = m_renderObjects[i];

		if (!obj->isHidden())
			return obj;
	}

	return nullptr;
}

shared_ptr<RenderObject> RenderObjectHolder::popFirstButton()
{
	for (int i = static_cast<int>(m_renderObjects.size()) - 1; i >= 0; --i)
	{
		shared_ptr<RenderObject> obj = m_renderObjects[i];

		if (obj->popActivated())
			return obj;
	}
	
	return nullptr;
}
		
shared_ptr<RenderObject> RenderObjectHolder::popFirstLeftClickButton()
{
	for (int i = static_cast<int>(m_renderObjects.size()) - 1; i >= 0; --i)
	{
		shared_ptr<RenderObject> obj = m_renderObjects[i];

		if (obj->popLeftClicked())
			return obj;
	}

	return nullptr;	
}

shared_ptr<RenderObject> RenderObjectHolder::popFirstRightClickButton()
{
	for (int i = static_cast<int>(m_renderObjects.size()) - 1; i >= 0; --i)
	{
		shared_ptr<RenderObject> obj = m_renderObjects[i];

		if (obj->popRightClicked())
			return obj;
	}

	return nullptr;	
}

shared_ptr<RenderObject> RenderObjectHolder::popKeyboundButton()
{
	for (int i = static_cast<int>(m_renderObjects.size()) - 1; i >= 0; --i)
	{
		shared_ptr<RenderObject> obj = m_renderObjects[i];

		if (obj->popKeyBound())
			return obj;
	}

	return nullptr;
}

void RenderObjectHolder::setPopupPromptDone(const int id)
{
	for (auto& itr : m_renderObjects)
	{
		if (auto obj = dynamic_pointer_cast<PopupPrompt>(itr))
		{
			if (obj->getId() == id)
			{
				m_finishedPromptBox = obj;
				removeObject(id);
				return;
			}
		}
	}

	blog(Logger::LOG_ERROR, "Setting confirm box as done did not find self.");
}
		
void RenderObjectHolder::registerContextMenuEx(const int id, const int childId, const sf::Vector2i pos, const vector<pair<string, sf::Color>>& lines)
{
	if (m_ctxMenuId != -1)
		return;

	auto obj = make_shared<ContextMenu>(*this, id, childId, pos);
	obj->setMaxLines(lines.size());

	for (auto& itr : lines)
		obj->addLine(itr.first, itr.second);

	addRenderObject(obj);
	m_ctxMenuId = id;
}

void RenderObjectHolder::registerContextMenu(const int id, const int childId, const sf::Vector2i pos, const vector<string>& lines)
{
	if (m_ctxMenuId != -1)
		return;

	auto obj = make_shared<ContextMenu>(*this, id, childId, pos);
	obj->setMaxLines(lines.size());

	for (auto& itr : lines)
		obj->addLine(itr);

	addRenderObject(obj);
	m_ctxMenuId = id;
}

void RenderObjectHolder::unregisterCtxMenu(const int id, const int childId, const string& result)
{
	if (auto ctxMenu = dynamic_pointer_cast<ContextMenu>(getRenderObject(m_ctxMenuId)))
	{
		if (auto reportTo = getRenderObject(ctxMenu->getReportToId()))
			reportTo->notifyCtxMenuClicked(id, result);

		destroyObjectById(m_ctxMenuId);
		m_ctxMenuId = -1;
	}
}

void RenderObjectHolder::setInputPrio(const int id)
{
	m_inputPrio = id;
}

void RenderObjectHolder::setConfirmBoxDone(const int id)
{
	for (auto& itr : m_renderObjects)
	{
		if (auto button = dynamic_pointer_cast<ConfirmMessageBox>(itr))
		{
			if (button->getId() == id)
			{
				m_finishedConfirmMessageBox = button;
				removeObject(id);
				return;
			}
		}
	}

	blog(Logger::LOG_ERROR, "Setting confirm box as done did not find self.");
}

shared_ptr<PopupPrompt> RenderObjectHolder::popPopupPrompt(const vector<int>& fishingFor)
{
	if (m_finishedPromptBox != nullptr)
	{
		if (!fishingFor.empty())
		{
			bool okay = false;

			for (auto& itr : fishingFor)
			{
				if (m_finishedPromptBox->getCode() != 0 && m_finishedPromptBox->getCode() == itr)
					okay = true;
			}

			if (!okay)
				return nullptr;
		}

		shared_ptr<PopupPrompt> result = m_finishedPromptBox;
		m_finishedPromptBox = nullptr;
		return result;
	}

	return nullptr;
}

shared_ptr<ConfirmMessageBox> RenderObjectHolder::popConfirmBox(const vector<int>& fishingFor)
{
	if (m_finishedConfirmMessageBox != nullptr)
	{
		if (!fishingFor.empty())
		{
			bool okay = false;

			for (auto& itr : fishingFor)
			{
				if (m_finishedConfirmMessageBox->getCode() != 0 && m_finishedConfirmMessageBox->getCode() == itr)
					okay = true;
			}

			if (!okay)
				return nullptr;
		}

		shared_ptr<ConfirmMessageBox> result = m_finishedConfirmMessageBox;
		m_finishedConfirmMessageBox = nullptr;
		return result;
	}

	return nullptr;
}

void RenderObjectHolder::toggleObjectHidden(const int id)
{
	if (auto obj = getRenderObject(id))
		obj->setHidden(!obj->isHidden());
}

void RenderObjectHolder::destroyObjectById(const int id)
{
	auto itr = m_renderObjects.begin(); 

	while (itr != m_renderObjects.end())
	{
		shared_ptr<RenderObject> data = *itr;

		if (data->getId() == id)
		{
			itr = m_renderObjects.erase(itr);
			m_pendingDestroys.insert(data);
		}
		else
		{
			++itr;
		}
	}
}

void RenderObjectHolder::clearNow()
{
	m_renderObjects.clear();
	m_pendingDestroys.clear();
}

void RenderObjectHolder::clear()
{
	auto itr = m_renderObjects.begin();

	while (itr != m_renderObjects.end())
	{
		shared_ptr<RenderObject> data = *itr;
		itr = m_renderObjects.erase(itr);
		m_pendingDestroys.insert(data);
	}
}

bool RenderObjectHolder::allowedMouseOver(const int id) const
{
	if (!m_multiInput)
		return true;

	if (!m_mousedOverObject)
		return true;

	return m_mousedOverObject == id;
}

bool RenderObjectHolder::popButtonId(const int id) const
{
	if (auto button = dynamic_pointer_cast<Button>(getRenderObject(id)))
		return button->popActivated();

	return false;
}

bool RenderObjectHolder::childMayInput(RenderObject const& obj) const
{
	if (m_multiInput)
		return true;

	// We allow ourselves to input
	if (&obj == this)
		return true;

	return getTopInput().get() == &obj;
}