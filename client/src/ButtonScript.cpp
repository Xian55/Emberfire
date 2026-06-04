#include "stdafx.h"
#include "ButtonScript.h"

ButtonScript::ButtonScript()
{

}

ButtonScript::~ButtonScript()
{

}

bool ButtonScript::parse(const vector<string>& lines)
{
	if (lines.size() < 4)
		return false;

	m_strTextureIdle = lines[1];
	m_strTextureHover = lines[2];
	m_strTexturePress = lines[3];

	if (lines.size() >= 5)
		m_strSoundPress = lines[4];

	return true;
}
