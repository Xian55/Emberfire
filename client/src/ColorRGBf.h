#pragma once

// 1.0f == 255
class ColorRGBf
{
	public:
		ColorRGBf(const float _r, const float _g, const float _b, const float _a);
		ColorRGBf();

		ColorRGBf operator- (const ColorRGBf &c) const;
		ColorRGBf operator+ (const ColorRGBf &c) const;
		ColorRGBf operator* (const ColorRGBf &c) const;
		ColorRGBf& operator-= (const ColorRGBf &c);
		ColorRGBf& operator+= (const ColorRGBf &c);
		ColorRGBf operator/ (const float scalar) const;
		ColorRGBf operator* (const float scalar) const;
		ColorRGBf& operator*= (const float scalar);

		bool operator== (const ColorRGBf &c) const;
		bool operator!= (const ColorRGBf &c) const;

		sf::Color toSf() const { return sf::Color(sf::Uint8(r * 255.0f), sf::Uint8(g * 255.0f), sf::Uint8(b * 255.0f), sf::Uint8(a * 255.0f)); }

	public:
		float r, g, b, a;
};

