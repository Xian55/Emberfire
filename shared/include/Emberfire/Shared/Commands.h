#pragma once
#include <cstdint>
#include <string>
#include <cstring>

// Console command dispatch framework. ConsoleWindow derives `protected Commands` and builds a
// CCommand[] table (terminated by an all-null entry). Sub-tables nest via the third field.
// Reconstructed from ConsoleWindow.cpp/.h usage.

class Commands;

// A single console command row. Either func (leaf) or subTable (group) is set.
struct CCommand
{
	const char* name;                              // command keyword (nullptr = end of table)
	bool (*func)(const char* args, Commands* thisptr); // handler; nullptr if this row is a group
	CCommand* subTable;                            // nested table; nullptr if this row is a leaf
};

class Commands
{
public:
	virtual ~Commands() = default;

	// Parse `text` (keyword [args...]), walk `table` (descending into subTables), invoke the
	// matching handler. Returns false if no command matched / handler failed.
	bool executeCommand(CCommand* table, const std::string& text)
	{
		if (table == nullptr)
			return false;

		// Split leading keyword from the remaining argument string.
		size_t sp = text.find(' ');
		const std::string word = (sp == std::string::npos) ? text : text.substr(0, sp);
		const std::string rest = (sp == std::string::npos) ? std::string() : text.substr(sp + 1);

		for (CCommand* c = table; c->name != nullptr; ++c)
		{
			if (word == c->name)
			{
				if (c->subTable != nullptr)
					return executeCommand(c->subTable, rest);
				if (c->func != nullptr)
					return c->func(rest.c_str(), this);
				return false;
			}
		}
		return false;
	}

	// Output sinks overridden by the host window (ConsoleWindow makes these `final`).
	virtual void error(const std::string& txt) {}
	virtual void warning(const std::string& txt) {}
};
