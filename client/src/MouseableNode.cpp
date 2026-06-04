#include "stdafx.h"
#include "MouseableNode.h"
#include "Application.h"
#include "..\Math.h"

MouseableNode::MouseableNode()
{

}

MouseableNode::~MouseableNode()
{

}

bool MouseableNode::isMousedOver(const bool useRealPosition /*= false*/)
{
	const auto& pos1 = getTopLeftCornerRef();
	const auto& pos2 = getBottomRightCornerRef();

	if (pos1.x == pos2.x && pos1.y == pos2.y)
		return false;

	bool useActualPos = m_bMouseOverUsesActualMousePos;

	if (useRealPosition)
		useActualPos = true;

	return Util::cordsInBox(sApplication->mousePos(useActualPos), pos1, pos2.x - pos1.x, pos2.y - pos1.y);
}
