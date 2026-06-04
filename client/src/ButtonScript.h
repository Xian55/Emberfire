#pragma once

class ButtonScript
{
	public:
		ButtonScript();
		~ButtonScript();

		bool parse(const vector<string>& lines);

		const auto& textureIdle() const { return m_strTextureIdle; }
		const auto& textureHover() const { return m_strTextureHover; }
		const auto& texturePress() const { return m_strTexturePress; }
		const auto& soundPress() const { return m_strSoundPress; }

	private:
		string m_strTextureIdle;
		string m_strTextureHover;
		string m_strTexturePress;
		string m_strSoundPress;
};

