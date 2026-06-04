#pragma once

#include "WorldPanel.h"

#include "..\Shared\GuildDefines.h"
#include "..\Shared\PlayerDefines.h"

class GuildRoster : public WorldPanel
{
	struct GuildMember
	{
		PlayerDefines::Classes classId;
		GuildDefines::Rank rank;
		int level;
		int guid;
		bool online;
	};

	enum CtxMenuOptions
	{
		DdoWhisper,
		DdoInvite,
		DdoPromote,
		DdoDemote,
		DdoKick,
		DdoCancel,
	};

	public:
		enum Interface
		{
			GuildTitle = 1,
			GuildMotd,
			EditMotdButton,
			ListNames,
			ListLevels,
			ListClasses,
			ShowOfflineTick,
			OnlineCount
		};

	public:
		GuildRoster(World& owner, const int id);
		virtual ~GuildRoster();

		void unsetGuild();
		void setGuild(const string& guildname);
		void setMotd(const string& str);
		void addMember(const int guid, const string& name, const PlayerDefines::Classes classId, const GuildDefines::Rank rank, const int level, const bool online = true);
		void setShowOffline(const bool v);
		void clearMemberList();

		/*virtual*/
		void onOpen() final;
		void notifyCtxMenuClicked(const int id, const string& lineClicked) final;

		bool hasMember(const string& name) const { return m_members.find(name) != m_members.end(); }
		bool isGuildSet() const { return getRenderObject(Interface::GuildTitle) != nullptr; }

		int getNumOnline() const { return m_numOnline; }

		GuildMember const* getMember(const string& name) const;

		GuildDefines::Rank getLocalRank() const;

	private:
		void input() final;
		void render() final;

		void setNumOnline(const int v);
		void redrawRoster();
		void addMemberToDrawLists(const unordered_map<string, GuildMember>::iterator& itr);

		bool isLeader() const { return getLocalRank() == GuildDefines::Rank::Leader; }

		static string ctxMenuOptionStr(const CtxMenuOptions id);

		bool m_showOffline;

		int m_numOnline;
		
		string m_guildName;
		string m_rightClickedName;

		// name, data
		//	Use string name as key since the rendered text list is a container of names
		unordered_map<string, GuildMember> m_members;
};

