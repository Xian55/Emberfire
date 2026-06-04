#pragma once

class Sprite;
class Text;

class TooltipGlyph
{
	public:
		enum Types
		{
			NullGlyph,
			Divider,
			Gems_x1,
			Gems_x2,
		};

	public:
		TooltipGlyph(const Types type, const int gem1 = 0, const int gem2 = 0, const string& gemFont = "", const int fontSize = 13);

		void draw(const sf::Vector2i pos, const int width) const;

		int getHeight() const;

		Types getType() const { return m_type; }

	private:
		const Types m_type;
		
		shared_ptr<Text> m_gem1_label;
		shared_ptr<Text> m_gem2_label;

		shared_ptr<Sprite> m_glyphDivider;
		shared_ptr<Sprite> m_glyphGemSlot1;
		shared_ptr<Sprite> m_glyphGemSlot2;
		shared_ptr<Sprite> m_glyphGemEmpty;
};

