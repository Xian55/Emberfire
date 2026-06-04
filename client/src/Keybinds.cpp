#include "stdafx.h"
#include "Keybinds.h"
#include "..\Shared\Config.h"
#include "..\Files.h"

Keybinds::Keybinds()
{

}

Keybinds::~Keybinds()
{

}

void Keybinds::load()
{
	vector<string> binds;
	Util::readLinesFromFile("scripts\\interface\\keybinds_1.txt", binds);
	Util::readLinesFromFile("scripts\\interface\\keybinds_2.txt", binds);
	
	for (auto& keybindName : binds)
		loadKeybind(keybindName);
}

void Keybinds::loadKeybind(const string& keybindName)
{
	const int keybind = sConfig->getInt("KeyBind", keybindName.c_str(), -1);
	const int keybindModifier = sConfig->getInt("KeyBindModifier", keybindName.c_str(), 0);

	SfKeyEvent ke;
	ke.clear();
	ke.code = sf::Keyboard::Key(keybind);

	if (keybindModifier == sf::Keyboard::LShift)
		ke.shift = true;
	else if (keybindModifier == sf::Keyboard::LAlt)
		ke.alt = true;
	else if (keybindModifier == sf::Keyboard::LControl)
		ke.control = true;

	m_keybinds[keybindName] = ke;
}

string Keybinds::deduceKeybindStr(const SfKeyEvent keyEvent) const
{
	int keybindModifier = 0;
	
	if (keyEvent.shift)
		keybindModifier = sf::Keyboard::LShift;
	else if (keyEvent.alt)
		keybindModifier = sf::Keyboard::LAlt;
	else if (keyEvent.control)
		keybindModifier = sf::Keyboard::LControl;

	return deduceKeybindStr(keyEvent.code, keybindModifier);
}

string Keybinds::deduceKeybindStr(const string& keybindName) const
{
	const int keybind = sConfig->getInt("KeyBind", keybindName.c_str(), -1);
	const int keybindModifier = sConfig->getInt("KeyBindModifier", keybindName.c_str(), 0);
	return deduceKeybindStr(keybind, keybindModifier);	
}

string Keybinds::deduceKeybindStr(const int keybind, const int keybindModifier) const
{
	if (keybind == -1)
		return "null";

	string result;

	if (keybindModifier == sf::Keyboard::LShift)
		result = "S+";
	else if (keybindModifier == sf::Keyboard::LAlt)
		result = "A+";
	else if (keybindModifier == sf::Keyboard::LControl)
		result = "C+";

	result += Util::sfKeyToString(sf::Keyboard::Key(keybind));
	return result;
}

SfKeyEvent Keybinds::getKeybindData(const string& keybindName) const
{
	auto itr = m_keybinds.find(keybindName);
	
	if (itr != m_keybinds.end())
		return itr->second;

	return SfKeyEvent();
}
