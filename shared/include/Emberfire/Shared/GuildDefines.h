#pragma once
#include <cstdint>
#include <string>
#include <cctype>

namespace GuildDefines
{
	// Rank ordinal: client compares rank > Rank::Initiate, casts wire ints to Rank, and also uses the
	// unscoped GuildDefines::Leader / GuildDefines::Officer forms -> plain enum. Higher = more authority.
	enum Rank
	{
		Initiate = 0, // TODO verify (lowest)
		Member = 1,   // TODO verify
		Officer = 2,  // TODO verify
		Leader = 3    // TODO verify (highest)
	};

	// Result of guild-name validation. Client compares == NameError::Success.
	enum class NameError
	{
		Success = 0,   // TODO verify
		TooShort = 1,  // TODO verify
		TooLong = 2,   // TODO verify
		Invalid = 3,   // TODO verify
		Taken = 4      // TODO verify
	};
}

// ---------------------------------------------------------------------------
// Reconstructed helper namespace (originally a transitively-included header).
// ---------------------------------------------------------------------------
namespace GuildFunctions
{
	// Display name for a guild rank.
	inline std::string rankName(GuildDefines::Rank rank)
	{
		switch (rank)
		{
		case GuildDefines::Rank::Initiate: return "Initiate";
		case GuildDefines::Rank::Member:   return "Member";
		case GuildDefines::Rank::Officer:  return "Officer";
		case GuildDefines::Rank::Leader:   return "Leader";
		default:                           return "Unknown";
		}
	}

	// True if 'actor' outranks 'target' and may promote/demote/kick them.
	// Leader can act on anyone below; Officer on Members/Initiates. TODO match server
	inline bool hasOfficerPowerOver(GuildDefines::Rank actor, GuildDefines::Rank target)
	{
		if (actor < GuildDefines::Rank::Officer)
			return false;
		return actor > target;
	}

	// Client-side guild-name validation. Returns NameError::Success when valid.
	// Server re-validates authoritatively. TODO match server
	inline GuildDefines::NameError invalidNameError(const std::string& name)
	{
		if ((int)name.size() < 3)
			return GuildDefines::NameError::TooShort;
		if ((int)name.size() > 24)
			return GuildDefines::NameError::TooLong;

		for (unsigned char c : name)
		{
			if (!(std::isalnum(c) || c == ' '))
				return GuildDefines::NameError::Invalid;
		}

		return GuildDefines::NameError::Success;
	}

	// Human-readable message for a guild-name validation result.
	inline std::string nameErrorStr(GuildDefines::NameError error)
	{
		switch (error)
		{
		case GuildDefines::NameError::Success:  return "Guild name is valid.";
		case GuildDefines::NameError::TooShort: return "That guild name is too short.";
		case GuildDefines::NameError::TooLong:  return "That guild name is too long.";
		case GuildDefines::NameError::Invalid:  return "That guild name contains invalid characters.";
		case GuildDefines::NameError::Taken:    return "That guild name is already taken.";
		default:                                return "That guild name cannot be used.";
		}
	}
}
