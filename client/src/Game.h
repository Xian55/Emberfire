#pragma once

#include "RenderObjectHolder.h"

class RenderObject;
class StlBuffer;

class Game : public RenderObjectHolder
{
	public:
		enum Ro
		{
			RoNull,
			RoLogin = 1,
			RoCharacterSelection,
			RoCharacterCreation,
			RoWorld,
			RoOptions,

			CtxMenu_DeleteCharacter,

			// Persistent Lua UI frame manager — a non-stage child that survives setStage (which only
			// destroys the active stage id), so the Lua UI covers every screen (login..world).
			RoLuaRoot
		};

	public:
		Game(RenderObjectHolder& owner, const int id);
		virtual ~Game();

		void toggleOptions(const bool v);
		void setStage(const Ro stage, const bool overrideDuplicate = false);

		static string getItemSound(const int itemId);
		static void playItemSound(const int itemId);

		Ro getStage() const { return m_stage; }

	private:
		void input() final;
		void render() final;

		void processPacket(StlBuffer& data);
		void processPacket_Mutual_Ping(StlBuffer& data);
		void processPacket_Server_Validate(StlBuffer& data);
		void processPacket_Server_QueuePosition(StlBuffer& data);
		void processPacket_Server_CharacterList(StlBuffer& data);
		void processPacket_Server_NewWorld(StlBuffer& data);
		void processPacket_Server_ChannelInfo(StlBuffer& data);
		void processPacket_Server_SetController(StlBuffer& data);
		void processPacket_Server_Player(StlBuffer& data);
		void processPacket_Server_Npc(StlBuffer& data);
		void processPacket_Server_GameObject(StlBuffer& data);
		void processPacket_Server_CharaCreateResult(StlBuffer& data);
		void processPacket_Server_Inventory(StlBuffer& data);
		void processPacket_Server_Bank(StlBuffer& data);
		void processPacket_Server_OpenBank(StlBuffer& data);
		void processPacket_Server_Spellbook(StlBuffer& data);
		void processPacket_Server_Spellbook_Update(StlBuffer& data);
		void processPacket_Server_EquipItem(StlBuffer& data);
		void processPacket_Server_WorldError(StlBuffer& data);
		void processPacket_Server_OfferDuel(StlBuffer& data);
		void processPacket_Server_LvlResponse(StlBuffer& data);
		void processPacket_Server_ChatError(StlBuffer& data);
		void processPacket_Server_ObjectVariable(StlBuffer& data);
		void processPacket_Server_UnitSpline(StlBuffer& data);
		void processPacket_Server_DestroyObject(StlBuffer& data);
		void processPacket_Server_ChatMsg(StlBuffer& data);
		void processPacket_Server_SetSubname(StlBuffer& data);
		void processPacket_Server_GuildRoster(StlBuffer& data);
		void processPacket_Server_GuildInvite(StlBuffer& data);
		void processPacket_Server_GuildAddMember(StlBuffer& data);
		void processPacket_Server_GuildRemoveMember(StlBuffer& data);
		void processPacket_Server_SpellGo(StlBuffer& data);
		void processPacket_Server_UnitTeleport(StlBuffer& data);
		void processPacket_Server_CombatMsg(StlBuffer& data);
		void processPacket_Server_InspectReveal(StlBuffer& data);
		void processPacket_Server_CastStart(StlBuffer& data);
		void processPacket_Server_CastStop(StlBuffer& data);
		void processPacket_Server_NotifyItemAdd(StlBuffer& data);
		void processPacket_Server_OpenLootWindow(StlBuffer& data);
		void processPacket_Server_UnitOrientation(StlBuffer& data);
		void processPacket_Server_UnitAuras(StlBuffer& data);
		void processPacket_Server_Cooldown(StlBuffer& data);
		void processPacket_Server_GossipMenu(StlBuffer& data);
		void processPacket_Server_AcceptedQuest(StlBuffer& data);
		void processPacket_Server_QuestList(StlBuffer& data);
		void processPacket_Server_QuestTally(StlBuffer& data);
		void processPacket_Server_QuestComplete(StlBuffer& data);
		void processPacket_Server_RewardedQuest(StlBuffer& data);
		void processPacket_Server_AbandonQuest(StlBuffer& data);
		void processPacket_Server_SpentGold(StlBuffer& data);
		void processPacket_Server_ExpNotify(StlBuffer& data);
		void processPacket_Server_AvailableWorldQuests(StlBuffer& data);
		void processPacket_Server_SocketResult(StlBuffer& data);
		void processPacket_Server_EmpowerResult(StlBuffer& data);
		void processPacket_Server_RespawnResponse(StlBuffer& data);
		void processPacket_Server_AggroMob(StlBuffer& data);
		void processPacket_Server_LearnedSpell(StlBuffer& data);
		void processPacket_Server_UpdateVendorStock(StlBuffer& data);
		void processPacket_Server_RepairCost(StlBuffer& data);
		void processPacket_Server_UnlockGameObj(StlBuffer& data);
		void processPacket_Server_QueryWaypointsResponse(StlBuffer& data);
		void processPacket_Server_DiscoverWaypointNotify(StlBuffer& data);
		void processPacket_Server_OfferParty(StlBuffer& data);
		void processPacket_Server_PartyList(StlBuffer& data);
		void processPacket_Server_OnObjectWasLooted(StlBuffer& data);
		void processPacket_Server_MarkNpcsOnMap(StlBuffer& data);
		void processPacket_Server_TradeUpdate(StlBuffer& data);
		void processPacket_Server_TradeCanceled(StlBuffer& data);
		void processPacket_Server_GuildOnlineStatus(StlBuffer& data);
		void processPacket_Server_GuildNotifyRoleChange(StlBuffer& data);
		void processPacket_Server_PromptRespec(StlBuffer& data);
		void processPacket_Server_ChannelChangeConfirm(StlBuffer& data);
		void processPacket_Server_ArenaReady(StlBuffer& data);
		void processPacket_Server_ArenaQueued(StlBuffer& data);
		void processPacket_Server_ArenaOutcome(StlBuffer& data);
		void processPacket_Server_ArenaStatus(StlBuffer& data);
		void processPacket_Server_PkNotify(StlBuffer& data);

		int m_guildInviteId;

		Ro m_stage;
		time_t m_serverTime;
};

