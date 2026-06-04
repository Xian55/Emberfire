#pragma once

#include "WorldChild.h"

#include "..\Geo2d.h"

class Sprite;
class Text;
class World;

class Minimap : public WorldChild
{
	enum Interface
	{
		RoBackground = 1,
		RoMinimapButton,
		RoLootbutton,
	};
	
	public:
		enum Defines
		{
			MapPixelWidth = 3200,
			MapPixelHeight = 1800
		};

		enum DotType
		{
			EnemyUnit,
			NeutralUnit,
			FriendlyUnit,
			DeadUnit,
			QuestStart,
			QuestComplete,
			GossipAvailable,
		};

	public:
		Minimap(World& owner, const int id);
		virtual ~Minimap();

		void registerDot(const int guid, const Geo2d::Vector2& worldPos, const Minimap::DotType dot);
		void setTitle(const string& zonename);
		void setChannelName(const string& channelName);
		void setMap(const string& mapname);
		void setMailLootStatus(const bool value);
		void setCachedChannelInfo(const int channelSizes, vector<int> channels);

		bool getMailLootStatus() const;
		
		static void mapRenderPosToMinimapPixelCord(float& x, float& y, const int mapWidth);
		static void minimapPixelCordToMapRenderPos(float& x, float& y, const int mapWidth);

	protected:
		void input() final;
		void render() final;
		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;

		Sprite* getDotSprite(const DotType dot);

		string m_mapname;
		
		shared_ptr<sf::Texture> m_mapDecal;
		shared_ptr<sf::Texture> m_minimapTexture;

		shared_ptr<Text> m_label;
		shared_ptr<Text> m_channelLabel;

		shared_ptr<Sprite> m_bgSpr;
		shared_ptr<Button> m_button;
		shared_ptr<Button> m_mailLootButton;
		
		shared_ptr<Sprite> m_dotEnemy;
		shared_ptr<Sprite> m_dotNeutral;
		shared_ptr<Sprite> m_dotFriendly;
		shared_ptr<Sprite> m_dotDead;
		shared_ptr<Sprite> m_dotQuestStart;
		shared_ptr<Sprite> m_dotQuestEnd;
		shared_ptr<Sprite> m_dotGossip;

		sf::Shader m_saturateShader;
		sf::RenderTexture m_renderTexture;

		int m_cachedChannelSizes = 0;
		vector<int> m_cachedChannels;

		map<int, pair<DotType, Geo2d::Vector2>> m_dots;
};

