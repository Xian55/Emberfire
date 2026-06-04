#pragma once
#include <cstdint>
#include <string>
#include <cctype>

namespace CharacterDefines
{
	// Result of character-name validation; Server_CharaCreateResult.m_result cast to this
	// (CharacterDefines::NameError(pk.m_result)) -> plain enum so the int cast is valid.
	enum NameError
	{
		Success = 0,        // TODO verify
		TooShort = 1,       // TODO verify
		TooLong = 2,        // TODO verify
		InvalidCharacters = 3, // TODO verify
		NameTaken = 4,      // TODO verify
		Profanity = 5       // TODO verify
	};

	namespace Names
	{
		static constexpr int MaxLength = 12; // TODO verify
	}

	namespace Portraits
	{
		static constexpr int NumMale = 32;   // TODO verify
		static constexpr int NumFemale = 32; // TODO verify
	}
}

// ---------------------------------------------------------------------------
// Reconstructed helper namespace (originally a transitively-included header).
// ---------------------------------------------------------------------------
namespace CharacterFunctions
{
	// Canonicalize a character/player name to "Capitalizedlowercase" form
	// (first letter upper, rest lower) and trim surrounding whitespace.
	inline std::string formatName(const std::string& raw)
	{
		std::string name = raw;

		// Trim leading/trailing whitespace.
		const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
		while (!name.empty() && !notSpace((unsigned char)name.front()))
			name.erase(name.begin());
		while (!name.empty() && !notSpace((unsigned char)name.back()))
			name.pop_back();

		for (std::size_t i = 0; i < name.size(); ++i)
		{
			unsigned char c = (unsigned char)name[i];
			name[i] = (i == 0) ? (char)std::toupper(c) : (char)std::tolower(c);
		}

		return name;
	}

	// Client-side name validation. Returns CharacterDefines::Success (== 0, falsey)
	// when valid, otherwise the offending error. Server re-validates authoritatively.
	// TODO match server
	inline CharacterDefines::NameError invalidNameError(const std::string& raw)
	{
		const std::string name = formatName(raw);

		if ((int)name.size() < 2)
			return CharacterDefines::TooShort;
		if ((int)name.size() > CharacterDefines::Names::MaxLength)
			return CharacterDefines::TooLong;

		for (unsigned char c : name)
		{
			if (!std::isalpha(c))
				return CharacterDefines::InvalidCharacters;
		}

		return CharacterDefines::Success;
	}

	// Human-readable message for a name validation result.
	inline std::string nameErrorStr(CharacterDefines::NameError error)
	{
		switch (error)
		{
		case CharacterDefines::Success:           return "Name is valid.";
		case CharacterDefines::TooShort:          return "That name is too short.";
		case CharacterDefines::TooLong:           return "That name is too long.";
		case CharacterDefines::InvalidCharacters: return "That name contains invalid characters.";
		case CharacterDefines::NameTaken:         return "That name is already taken.";
		case CharacterDefines::Profanity:         return "That name is not allowed.";
		default:                                  return "That name cannot be used.";
		}
	}
}
