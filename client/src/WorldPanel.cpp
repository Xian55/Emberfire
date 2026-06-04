#include "stdafx.h"
#include "WorldPanel.h"
#include "World.h"
#include "Button.h"
#include "SpriteRo.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "Application.h"

#include <assert.h>

WorldPanel::WorldPanel(World& owner, const int id, const string& background, const sf::Vector2i& closeButtonOffset, const string& closeButtonName) :
	WorldChild(owner, id, owner),
	m_world(owner)
{
	// Panels always hidden by default
	setHidden(true);
	setMultiInput(true);

	// Background
	auto spr = sContentMgr->spawnSprite(background);
	auto bg = make_shared<SpriteRo>(*this, spr, Interface::Background);
	bg->setAnchor(&getTopLeftCornerRef());
	addRenderObject(bg);
	m_size = sf::Vector2i(spr->getTexture()->getSize());

	// Style
	getTopLeftCornerRef().y = worldPanelYPos();
	getBottomRightCornerRef() = getTopLeftCornerRef() + m_size;

	// Close button, magic number -99 because bleh
	auto closeButton = make_shared<Button>(*this, closeButtonName, Interface::CloseButton);
	closeButton->setAnchor(&getTopLeftCornerRef());
	closeButton->setOffset(closeButtonOffset);
	closeButton->setKeyEvent(SfKeyEvent(sf::Keyboard::Escape));
	addRenderObject(closeButton);
}

WorldPanel::~WorldPanel()
{

}

void WorldPanel::input()
{
	__super::input();

	// bleh
	if (popButtonId(Interface::CloseButton))
		m_world.closePanel(World::Interface(getId()));
	else
		requestMoveToTop(Interface::CloseButton);
}

int WorldPanel::worldPanelYPos() const 
{
	return int(sApplication->sHf() * 0.1333f);
}

void WorldPanel::render()
{
	__super::render();
	getBottomRightCornerRef() = getTopLeftCornerRef() + m_size;
}

void WorldPanel::alterBackground(const string& textureName)
{
	auto newSpr = sContentMgr->spawnSprite(textureName);
	ASSERT(newSpr != nullptr);

	if (auto bg = dynamic_pointer_cast<SpriteRo>(getRenderObject(Interface::Background)))
		bg->changeSprite(newSpr);
}
