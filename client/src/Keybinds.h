#pragma once

class Keybinds
{
	public:
		static Keybinds* instance()
		{
			static Keybinds c;
			return &c;
		}

		void load();
		void loadKeybind(const string& keybindName);

		string deduceKeybindStr(const SfKeyEvent keyEvent) const;
		string deduceKeybindStr(const string& keybindName) const;
		string deduceKeybindStr(const int keybind, const int modifier) const;

		SfKeyEvent getKeybindData(const string& keybindName) const;

	private:
		Keybinds();
		~Keybinds();

		unordered_map<string, SfKeyEvent> m_keybinds;
};

#define sKeybinds Keybinds::instance()

