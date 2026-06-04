#include "stdafx.h"
#include "ColorRGBf.h"

ColorRGBf::ColorRGBf(const float _r, const float _g, const float _b, const float _a)
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;
}

ColorRGBf::ColorRGBf() :
	r(0.0f),
	g(0.0f),
	b(0.0f),
	a(0.0f)
{

}

ColorRGBf ColorRGBf::operator-(const ColorRGBf &c) const
{
	return ColorRGBf(r - c.r, g - c.g, b - c.b, a - c.a);
}

ColorRGBf ColorRGBf::operator+(const ColorRGBf &c) const
{
	return ColorRGBf(r + c.r, g + c.g, b + c.b, a + c.a);
}

ColorRGBf ColorRGBf::operator/(const float scalar) const
{
	return ColorRGBf(r / scalar, g / scalar, b / scalar, a / scalar);
}

ColorRGBf ColorRGBf::operator*(const ColorRGBf &c) const
{
	return ColorRGBf(r*c.r, g*c.g, b*c.b, a*c.a);
}

ColorRGBf ColorRGBf::operator*(const float scalar) const
{
	return ColorRGBf(r * scalar, g * scalar, b * scalar, a * scalar);
}

ColorRGBf& ColorRGBf::operator*=(const float scalar)
{
	r *= scalar;
	g *= scalar;
	b *= scalar; 
	a *= scalar; 
	return *this;
}

ColorRGBf& ColorRGBf::operator+=(const ColorRGBf &c)
{
	r += c.r; 
	g += c.g; 
	b += c.b; 
	a += c.a; 
	return *this;
}

ColorRGBf& ColorRGBf::operator-=(const ColorRGBf &c)
{
	r -= c.r;
	g -= c.g;
	b -= c.b;
	a -= c.a;
	return *this;
}

bool ColorRGBf::operator!=(const ColorRGBf &c) const
{
	return (r != c.r || g != c.g || b != c.b || a != c.a);
}

bool ColorRGBf::operator==(const ColorRGBf &c) const
{
	return (r == c.r && g == c.g && b == c.b && a == c.a);
}