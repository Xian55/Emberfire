#pragma once
#include <cstdint>

namespace ChatDefines
{
	// Wire channel id. Client uses both ChatDefines::Channels::Say (scoped, in switch/case) and
	// ChatDefines::Say (unscoped), and compares an int m_channelId directly against Channels values,
	// so this is a plain enum. Values must match server (carried in Server_ChatMsg.channel:u8).
	enum Channels
	{
		// DEFINITIVE from capture (C2S op17 byte0 / S2C op91 byte4): Say/Yell/Whisper confirmed,
		// Guild=3 & Party=4 (were swapped), System=5 (roll/"Success."), AllChat=6 (global).
		Say = 0,
		Yell = 1,
		Whisper = 2,
		Guild = 3,        // was 4
		Party = 4,        // was 3
		System = 5,       // was 6 (roll results, guild-create "Success.")
		AllChat = 6,      // global/world (was 5)
		SystemCenter = 7, // TODO verify (centered scrolling message)
		ExpPurple = 8,    // notice (durability/exp); seen as channel 8
		RedWarning = 9,   // confirmed ("You must be at least level 25…")
		NumChannels
	};
}
