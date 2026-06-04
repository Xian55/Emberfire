#include "stdafx.h"
#include "RenderObject.h"
#include "RenderObjectHolder.h"
#include "Application.h"
#include "PromptBox.h"

#include <assert.h>

RenderObject::RenderObject(RenderObject* owner, const int id /*= 0*/) :
	AnchoredPosition(sf::Vector2i(), &m_topLeftCorner),
	m_isHidden(false),
	m_id(id),
	m_owner(owner),
	m_mouseable(true)
{

}

RenderObject::~RenderObject()
{

}
	
bool RenderObject::wasInputLastFrame() const
{
	return m_lastInputFrame == sApplication->getLastFrameId();
}

void RenderObject::attemptInput()
{
	if (!mayInput())
		return;

	input();
	m_lastInputFrame = sApplication->getCurrentFrameId();
}

void RenderObject::attemptRender()
{
	//if (dynamic_cast<PromptBox*>(this) != 0)
	//	;

	updateAnchoredPosition();

	if (!isHidden())
	{
		// The last object to call this function wins
		if (m_mouseable && MouseableNode::isMousedOver(true))
			sApplication->setTopMostMousedOver(this);

		render();
	}
}

bool RenderObject::isChildOf(RenderObject& obj)
{
	RenderObject const* myself = this;
	RenderObject const* parent = m_owner;

	for (int i = 0; i < 4096; ++i)
	{
		if (myself == &obj)
			return true;

		if (parent == &obj)
			return true;

		// End of the line
		if (myself == parent)
			return false;

		myself = parent;
		parent = parent->m_owner;
	}

	// Infinite loop occurred
	ASSERT(0);
	return false;
}

bool RenderObject::mayInput() const
{
	if (isHidden())
		return false;

	RenderObject const* myself = this;
	RenderObject const* parent = m_owner;

	for (int i = 0; i < 4096; ++i)
	{
		// End of the line
		if (myself == parent)
			return true;

		if (!parent->mayInput())
			return false;

		if (parent->childMayInput(*this))
			return true;

		myself = parent;
		parent = parent->m_owner;
	}

	// Infinite loop occurred
	ASSERT(0);
	return false;
}

bool RenderObject::isMousedOver(const bool useRealPosition /*= false*/)
{
	//RenderObject* pObj = (RenderObject*)sApplication->getTopMostMousedOver();

	if (sApplication->getTopMostMousedOver() != reinterpret_cast<uintptr_t>(this))
		return false;

	return MouseableNode::isMousedOver(useRealPosition);
}

void RenderObject::initOwner(RenderObject* owner)
{
	ASSERT(m_owner == nullptr);
	m_owner = owner;
}
