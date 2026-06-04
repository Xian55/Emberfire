#include "stdafx.h"
#include "ConsoleWindow.h"
#include "Application.h"
#include "DraggableNode.h"
#include "Button.h"
#include "PromptBox.h"
#include "ScrollBar.h"
#include "MapEditor.h"
#include "ClientMap.h"
#include "ClientNpc.h"
#include "Game.h"
#include "WorldSpellAnimation.h"
#include "ContentMgr.h"
#include "DbTemplateEditor.h"
#include "MapCellClient.h"
#include "Sprite.h"

#include "..\Files.h"
#include "..\StringHelpers.h"

#include <sstream>

ConsoleWindow::ConsoleWindow(RenderObject& owner, const int id) :
	ExpandableWindow(owner, id,
		"console_top_right.png",
		"console_top_across.png",
		"console_top_left.png",
		"console_bottom_right.png",
		"console_bottom_across.png",
		"console_bottom_left.png",
		"console_left_up.png",
		"console_right_up.png",
		"console_center.png")
{
	setHidden(true);
	m_bottomRightCorner = { sApplication->sW() / 2, sApplication->sH() / 2 };

	// This allows us to be draggable
	m_dragNode = make_unique<DraggableNode>();
	m_dragNode->addAnchor(&m_topLeftCorner);
	m_dragNode->addAnchor(&m_bottomRightCorner);

	// This is the 'X' button that closes the window
	m_closeButton = make_unique<Button>(*this, "console_button_close", 0, sf::Vector2i(), SfKeyEvent(sf::Keyboard::Escape));
	m_closeButton->setAnchor(&m_topRightCorner);
	m_closeButton->setOffset({ -26, 8 });
	
	shared_ptr<ScrollBar> scrollBar = make_shared<ScrollBar>(*this, "console_scroll_up", "console_scroll_down", ScrollBar::ScrollBottomUp, "console_scroll_bar");
	scrollBar->getScrollDownButton()->setAnchor(&m_bottomRightCorner);
	scrollBar->getScrollDownButton()->setOffset(sf::Vector2i(-33, -63));
	scrollBar->getScrollUpButton()->setAnchor(&m_topRightCorner);
	scrollBar->getScrollUpButton()->setOffset(sf::Vector2i(-33, 39));

	// Given to the prompt box
	shared_ptr<Button> enterButton = make_shared<Button>(*this, "console_enter_button", 0, sf::Vector2i(), SfKeyEvent(sf::Keyboard::Return));
	enterButton->setAnchor(&m_bottomRightCorner);
	enterButton->setOffset(sf::Vector2i(-87, -38));

	// PromptBox class will destruct the ScrollBar for us, which destroys the two buttons
	m_promptBox = make_shared<PromptBox>(*this, 0, "Helvetica 400.ttf", enterButton, sf::Vector2i(), 100, sf::Color::White);
	m_promptBox->setDialogCharacterSize(14);
	m_promptBox->setPromptCharacterSize(12);
	m_promptBox->setScrollObject(scrollBar);
	m_promptBox->setLeading(2);

	executeFile("autoexec.conf");
}

ConsoleWindow::~ConsoleWindow()
{

}

void ConsoleWindow::input() /*final*/
{
	sApplication->setCurrentPrompt(m_promptBox.get());

	__super::input();

	m_dragNode->updateDraggableObject(*this);
	m_promptBox->attemptInput();
	m_closeButton->attemptInput();

	if (m_closeButton->popActivated())
		setHidden(true);

	const string poppedInput = m_promptBox->popQueuedStatement();

	if (poppedInput.size() > 0)
		performCommand(poppedInput);
}

void ConsoleWindow::render() /*final*/
{
	__super::render();

	// Resize text box based on window dimensions.
	m_promptBox->getTopLeftCornerRef() = { m_topLeftCorner.x + 20, m_bottomRightCorner.y - 35 };
	m_promptBox->setBounds(Util::GeoBox2d(0, -(getCenterHeight() + 120), static_cast<int>(getCenterWidth()) + 100, static_cast<int>(getCenterHeight()) + 85));
	m_promptBox->setMaxWidth(75 + static_cast<int>(getCenterWidth()));

	m_promptBox->attemptRender();
	m_closeButton->attemptRender();

	if (RenderObjectHolder* pHolder = dynamic_cast<RenderObjectHolder*>(getOwner()))
		pHolder->requestMoveToTop(getId());
}

void ConsoleWindow::setHidden(const bool b)
{
	__super::setHidden(b);
}

void ConsoleWindow::performCommand(const string& text)
{
	m_promptBox->addLine("[CMD]: " + text);

	if (!executeCommand(getCommandTable(), text))
		error("Failed to execute command `" + text + "`.");
}

void ConsoleWindow::error(const string& txt) /*final*/
{
	m_promptBox->addLine(txt.c_str(), sf::Color(ConsoleColors::ErrorRed));
}

void ConsoleWindow::warning(const string& txt) /*final*/
{
	m_promptBox->addLine(txt.c_str(), sf::Color(ConsoleColors::WarningYellow));
}

void ConsoleWindow::executeFile(const string& filename)
{
	blog(Logger::LOG_INFO, "Executing %s.", filename.c_str());

	vector<string> lines;
	Util::readLinesFromFile(filename, lines);
	std::for_each(lines.begin(), lines.end(), [&](const string& str) { performCommand(str); });
}

CCommand* ConsoleWindow::getCommandTable()
{
	// Name					Handler											Child Commands
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static CCommand mapEditorTable[] =
	{
		{ "name",           &ConsoleWindow::handleMapEditorSetName,					nullptr },
		{ "toggle",			&ConsoleWindow::handleMapEditorToggle,					nullptr },
		{ "load",			&ConsoleWindow::handleMapEditorLoad,					nullptr },
		{ "save",			&ConsoleWindow::handleMapEditorSave,					nullptr },
		{ "new",			&ConsoleWindow::handleMapEditorNew,						nullptr },
		{ "dummynpc",		&ConsoleWindow::handleMapEditorSpawnExampleNpc,			nullptr },
		{ "dummyspellanim",	&ConsoleWindow::handleMapEditorSpawnExampleSpellAnim,	nullptr },
		{ nullptr,          nullptr,												nullptr }
	};
	
	static CCommand dbEditorTable[] =
	{
		{ "toggle",			&ConsoleWindow::handleDbEditorToggle,					nullptr },
		{ "stage",			&ConsoleWindow::handleDbEditorStage,					nullptr },
		{ nullptr,          nullptr,												nullptr }
	};
	
	static CCommand gameCmdTable[] =
	{
		{ "toggle",			&ConsoleWindow::handleGameToggle,				nullptr },
		{ "stage",			&ConsoleWindow::handleGameSetStage,				nullptr },
		{ nullptr,          nullptr,										nullptr }
	};

	static CCommand reloadTable[] =
	{
		{ "contentmgr",		&ConsoleWindow::handleReloadContentMgr,			nullptr },
		{ nullptr,          nullptr,										nullptr }
	};

	static CCommand dumpTable[] =
	{
		{ "textures",		&ConsoleWindow::handleDumpTextures,				nullptr },
		{ nullptr,          nullptr,										nullptr }
	};

	static CCommand commandTable[] =
	{
		{ "quit",			&ConsoleWindow::handleQuitCmd,					nullptr },
		{ "dbeditor",		&ConsoleWindow::handleDbEditorToggle,			dbEditorTable },
		{ "mapeditor",		nullptr,										mapEditorTable },
		{ "game",			nullptr,										gameCmdTable },
		{ "reload",			nullptr,										reloadTable },
		{ "dump",			nullptr,										dumpTable },
		{ "delta",			&ConsoleWindow::handleDeltaCommand,				nullptr },
		{ nullptr,          nullptr,										nullptr }
	};

	return commandTable;
}

bool ConsoleWindow::handleQuitCmd(const char* args, Commands* thisptr)
{
	sApplication->cancel();
	return true;
}

bool ConsoleWindow::handleMapEditorSave(const char* args, Commands* thisptr)
{
	auto ro = sApplication->getRenderObject(Application::RoMapEditor);

	if (ro == nullptr)
	{
		thisptr->warning("The map editor is not open.");
		return false;
	}

	if (auto mapEditor = dynamic_pointer_cast<MapEditor>(ro))
	{
		if (mapEditor->getMap() == nullptr)
		{
			thisptr->warning("Not presently editing a map.");
			return false;
		}

		mapEditor->getMap()->saveToDisk();
		return true;
	}

	// Why the heck can't we cast into the map editor??
	ASSERT(0);
	return true;
}

bool ConsoleWindow::handleMapEditorLoad(const char* args, Commands* thisptr)
{
	auto ro = sApplication->getRenderObject(Application::RoMapEditor);

	if (ro == nullptr)
	{
		thisptr->warning("The map editor is not open.");
		return false;
	}

	if (auto mapEditor = dynamic_pointer_cast<MapEditor>(ro))
	{
		if (mapEditor->getMap() == nullptr)
		{
			thisptr->warning("Not presently editing a map.");
			return false;
		}

		if (!mapEditor->getMap()->loadFromDisk(args))
			return false;

		mapEditor->initMap();
		mapEditor->initMinimap(args);
		return true;
	}

	// Why the heck can't we cast into the map editor??
	ASSERT(0);
	return true;
}

bool ConsoleWindow::handleMapEditorNew(const char* args, Commands* thisptr)
{
	auto ro = sApplication->getRenderObject(Application::RoMapEditor);

	if (ro == nullptr)
	{
		thisptr->warning("The map editor is not open.");
		return false;
	}

	if (auto mapEditor = dynamic_pointer_cast<MapEditor>(ro))
	{
		if (mapEditor->getMap() == nullptr)
		{
			thisptr->warning("Not presently editing a map.");
			return false;
		}

		int isize = atoi(args);

		if (isize <= 0)
		{
			thisptr->warning("Invalid size");
			return false;
		}

		mapEditor->getMap()->resize(isize);
		return true;
	}

	// Why the heck can't we cast into the map editor??
	ASSERT(0);
	return true;
}

bool ConsoleWindow::handleMapEditorSetName(const char* args, Commands* thisptr)
{
	auto ro = sApplication->getRenderObject(Application::RoMapEditor);

	if (ro == nullptr)
	{
		thisptr->warning("The map editor is not open.");
		return false;
	}

	if (auto mapEditor = dynamic_pointer_cast<MapEditor>(ro))
	{
		if (mapEditor->getMap() == nullptr)
		{
			thisptr->warning("Not presently editing a map.");
			return false;
		}

		mapEditor->getMap()->setName(args);
		return true;
	}

	// Why the heck can't we cast into the map editor??
	ASSERT(0);
	return true;
}

bool ConsoleWindow::handleMapEditorSpawnExampleSpellAnim(const char* args, Commands* thisptr)
{
	auto ro = sApplication->getRenderObject(Application::RoMapEditor);

	if (ro == nullptr)
	{
		thisptr->warning("The map editor is not open.");
		return false;
	}

	if (auto mapEditor = dynamic_pointer_cast<MapEditor>(ro))
	{
		if (mapEditor->getMap() == nullptr)
		{
			thisptr->warning("Not presently editing a map.");
			return false;
		}

		vector<string> argsSplit;
		Util::splitStr(args, argsSplit, ' ');

		if (argsSplit.size() < 4)
			return false;

		const int spellId = atoi(argsSplit[0].c_str());
		const float speed = static_cast<float>(atof(argsSplit[1].c_str()));
		const float targetX = static_cast<float>(atof(argsSplit[2].c_str()));
		const float targetY = static_cast<float>(atof(argsSplit[3].c_str()));

		shared_ptr<WorldSpellAnimation> sa = make_shared<WorldSpellAnimation>(mapEditor->getMap().get(), spellId);
		sa->setSpeed(speed);
		sa->setTarget({ targetX, targetY });
		mapEditor->getMap()->addWorldRenderable(sa);
		return true;
	}

	// Why the heck can't we cast into the map editor??
	ASSERT(0);
	return true;
}

bool ConsoleWindow::handleReloadContentMgr(const char* args, Commands* thisptr)
{
	sContentMgr->reload();
	return true;
}

bool ConsoleWindow::handleDumpTextures(const char* args, Commands* thisptr)
{
	sContentMgr->dumpTextures();
	return true;
}

bool ConsoleWindow::handleDeltaCommand(const char* args, Commands* thisptr)
{
	thisptr->warning("Delta: " + to_string(sApplication->delta()));
	return true;
}

bool ConsoleWindow::handleMapEditorSpawnExampleNpc(const char* args, Commands* thisptr)
{
	thisptr->warning("Deprecated");
	return true;
}

bool ConsoleWindow::handleGameSetStage(const char* args, Commands* thisptr)
{
	Game::Ro stage;

	if (string(args) == "login")
	{
		stage = Game::Ro::RoLogin;
	}
	else if (string(args) == "selection")
	{
		stage = Game::Ro::RoCharacterSelection;
	}
	else if (string(args) == "creation")
	{
		stage = Game::Ro::RoCharacterCreation;
	}
	else if (string(args) == "world")
	{
		stage = Game::Ro::RoWorld;
	}
	else
	{
		thisptr->warning("Invalid argument, try login/world/creation/selection");
		return false;
	}

	sApplication->destroyObjectById(Application::RoMapEditor);
	sApplication->destroyObjectById(Application::RoDbEditor);

	auto ro = dynamic_pointer_cast<Game>(sApplication->getRenderObject(Application::RoGame));

	if (ro == nullptr)
	{
		ro = make_shared<Game>(*sApplication, Application::RoGame);
		sApplication->addRenderObject(ro);
	}

	ro->setStage(stage);
	return true;
}

bool ConsoleWindow::handleGameToggle(const char* args, Commands* thisptr)
{
	if (strlen(args) == 0)
	{
		thisptr->warning("1 to enable, 0 to disable");
		return false;
	}

	// todo: editing in-game map while in-game ?
	sApplication->destroyObjectById(Application::RoMapEditor);

	if (atoi(args) != 0)
	{
		if (sApplication->getRenderObject(Application::RoGame) != nullptr)
		{
			thisptr->warning("Already exists.");
			return false;
		}

		sApplication->addRenderObject(make_shared<Game>(*sApplication, Application::RoGame));
	}
	else
	{
		sApplication->destroyObjectById(Application::RoGame);
	}

	return true;
}

bool ConsoleWindow::handleDbEditorStage(const char* args, Commands* thisptr)
{	
	auto ro = dynamic_pointer_cast<DbTemplateEditor>(sApplication->getRenderObject(Application::RoDbEditor));

	if (ro == nullptr)
	{
		thisptr->warning("DbEditor is not open.");
		return false;
	}

	return ro->setStage(string(args));
}

bool ConsoleWindow::handleDbEditorToggle(const char* args, Commands* thisptr)
{
	if (strlen(args) == 0)
	{
		thisptr->warning("1 to enable, 0 to disable");
		return false;
	}

	sApplication->destroyObjectById(Application::RoGame);
	sApplication->destroyObjectById(Application::RoMapEditor);

	if (atoi(args) != 0)
	{
		if (sApplication->getRenderObject(Application::RoDbEditor) != nullptr)
		{
			thisptr->warning("Already exists.");
			return false;
		}

		sApplication->addRenderObject(make_shared<DbTemplateEditor>(*sApplication, Application::RoDbEditor));
	}
	else
	{
		sApplication->destroyObjectById(Application::RoDbEditor);
	}

	return true;
}

bool ConsoleWindow::handleMapEditorToggle(const char* args, Commands* thisptr)
{
	if (strlen(args) == 0)
	{
		thisptr->warning("1 to enable, 0 to disable");
		return false;
	}

	sApplication->destroyObjectById(Application::RoGame);
	sApplication->destroyObjectById(Application::RoDbEditor);

	if (atoi(args) != 0)
	{
		if (sApplication->getRenderObject(Application::RoMapEditor) != nullptr)
		{
			thisptr->warning("Already exists.");
			return false;
		}

		sApplication->addRenderObject(make_shared<MapEditor>(*sApplication, Application::RoMapEditor));
	}
	else
	{
		sApplication->destroyObjectById(Application::RoMapEditor);
	}

	return true;
}