#pragma once
#include <cstdint>

namespace AccountDefines
{
	// Server_Validate.m_result is read as this enum (Game.cpp:365 switch). result==0 => success (capture).
	// Client only references BadPassword/Banned/ServerFull/Validated/WrongVersion by name.
	// Validated must == 0 (capture: Server_Validate result=0 on success).
	enum class AuthenticateResult : uint32_t
	{
		Validated = 0,    // confirmed (capture: result 0 = ok)
		WrongVersion = 1, // TODO verify ordinal
		BadPassword = 2,  // TODO verify
		ServerFull = 3,   // TODO verify
		Banned = 4        // TODO verify
	};

	static constexpr int MaxUserLength = 32; // TODO verify
	static constexpr int MaxPassLength = 32; // TODO verify
}
