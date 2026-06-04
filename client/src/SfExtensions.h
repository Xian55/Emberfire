#pragma once

#include <memory>
#include <unordered_map>

namespace Util
{
	static sf::Color brightenColor(const sf::Color color, const float correctionFactor = 0.5f)
	{
		const float red = (255 - color.r) * correctionFactor + color.r;
		const float green = (255 - color.g) * correctionFactor + color.g;
		const float blue = (255 - color.b) * correctionFactor + color.b;
		return sf::Color((sf::Uint8)red, (sf::Uint8)green, (sf::Uint8)blue, color.a);
	}

	static string sfKeyToString(sf::Keyboard::Key keycode)
	{
		switch (keycode) 
		{
			case sf::Keyboard::A: return "A"; break;
			case sf::Keyboard::B: return "B"; break;
			case sf::Keyboard::C: return "C"; break;
			case sf::Keyboard::D: return "D"; break;
			case sf::Keyboard::E: return "E"; break;
			case sf::Keyboard::F: return "F"; break;
			case sf::Keyboard::G: return "G"; break;
			case sf::Keyboard::H: return "H"; break;
			case sf::Keyboard::I: return "I"; break;
			case sf::Keyboard::J: return "J"; break;
			case sf::Keyboard::K: return "K"; break;
			case sf::Keyboard::L: return "L"; break;
			case sf::Keyboard::M: return "M"; break;
			case sf::Keyboard::N: return "N"; break;
			case sf::Keyboard::O: return "O"; break;
			case sf::Keyboard::P: return "P"; break;
			case sf::Keyboard::Q: return "Q"; break;
			case sf::Keyboard::R: return "R"; break;
			case sf::Keyboard::S: return "S"; break;
			case sf::Keyboard::T: return "T"; break;
			case sf::Keyboard::U: return "U"; break;
			case sf::Keyboard::V: return "V"; break;
			case sf::Keyboard::W: return "W"; break;
			case sf::Keyboard::X: return "X"; break;
			case sf::Keyboard::Y: return "Y"; break;
			case sf::Keyboard::Z: return "Z"; break;
			case sf::Keyboard::Num0: return "0"; break;
			case sf::Keyboard::Num1: return "1"; break;
			case sf::Keyboard::Num2: return "2"; break;
			case sf::Keyboard::Num3: return "3"; break;
			case sf::Keyboard::Num4: return "4"; break;
			case sf::Keyboard::Num5: return "5"; break;
			case sf::Keyboard::Num6: return "6"; break;
			case sf::Keyboard::Num7: return "7"; break;
			case sf::Keyboard::Num8: return "8"; break;
			case sf::Keyboard::Num9: return "9"; break;
			case sf::Keyboard::Escape: return "Escape"; break;
			case sf::Keyboard::LControl: return "LControl"; break;
			case sf::Keyboard::LShift: return "LShift"; break;
			case sf::Keyboard::LAlt: return "LAlt"; break;
			case sf::Keyboard::LSystem: return "LSystem"; break;
			case sf::Keyboard::RControl: return "RControl"; break;
			case sf::Keyboard::RShift: return "RShift"; break;
			case sf::Keyboard::RAlt: return "RAlt"; break;
			case sf::Keyboard::RSystem: return "RSystem"; break;
			case sf::Keyboard::Menu: return "Menu"; break;
			case sf::Keyboard::LBracket: return "LBracket"; break;
			case sf::Keyboard::RBracket: return "RBracket"; break;
			case sf::Keyboard::SemiColon: return ";"; break;
			case sf::Keyboard::Comma: return ","; break;
			case sf::Keyboard::Period: return "."; break;
			case sf::Keyboard::Quote: return "\'"; break;
			case sf::Keyboard::Slash: return "/"; break;
			case sf::Keyboard::BackSlash: return "\\"; break;
			case sf::Keyboard::Tilde: return "~"; break;
			case sf::Keyboard::Equal: return "="; break;
			case sf::Keyboard::Dash: return "-"; break;
			case sf::Keyboard::Space: return "Space"; break;
			case sf::Keyboard::Return: return "Return"; break;
			case sf::Keyboard::BackSpace: return "Back"; break;
			case sf::Keyboard::Tab: return "Tab"; break;
			case sf::Keyboard::PageUp: return "Page Up"; break;
			case sf::Keyboard::PageDown: return "Page Down"; break;
			case sf::Keyboard::End: return "End"; break;
			case sf::Keyboard::Home: return "Home"; break;
			case sf::Keyboard::Insert: return "Insert"; break;
			case sf::Keyboard::Delete: return "Delete"; break;
			case sf::Keyboard::Add: return "+"; break;
			case sf::Keyboard::Subtract: return "-"; break;
			case sf::Keyboard::Multiply: return "*"; break;
			case sf::Keyboard::Divide: return "/"; break;
			case sf::Keyboard::Left: return "Left"; break;
			case sf::Keyboard::Right: return "Right"; break;
			case sf::Keyboard::Up: return "UP"; break;
			case sf::Keyboard::Down: return "Down"; break;
			case sf::Keyboard::Numpad0: return "NP0"; break;
			case sf::Keyboard::Numpad1: return "NP1"; break;
			case sf::Keyboard::Numpad2: return "NP2"; break;
			case sf::Keyboard::Numpad3: return "NP3"; break;
			case sf::Keyboard::Numpad4: return "NP4"; break;
			case sf::Keyboard::Numpad5: return "NP5"; break;
			case sf::Keyboard::Numpad6: return "NP6"; break;
			case sf::Keyboard::Numpad7: return "NP7"; break;
			case sf::Keyboard::Numpad8: return "NP8"; break;
			case sf::Keyboard::Numpad9: return "NP9"; break;
			case sf::Keyboard::F1: return "F1"; break;
			case sf::Keyboard::F2: return "F2"; break;
			case sf::Keyboard::F3: return "F3"; break;
			case sf::Keyboard::F4: return "F4"; break;
			case sf::Keyboard::F5: return "F5"; break;
			case sf::Keyboard::F6: return "F6"; break;
			case sf::Keyboard::F7: return "F7"; break;
			case sf::Keyboard::F8: return "F8"; break;
			case sf::Keyboard::F9: return "F9"; break;
			case sf::Keyboard::F10: return "F10"; break;
			case sf::Keyboard::F11: return "F11"; break;
			case sf::Keyboard::F12: return "F12"; break;

			// Hacks
			case sf::Keyboard::F13: return "M3";
			case sf::Keyboard::F14: return "M4";
			case sf::Keyboard::F15: return "M5";
			case sf::Keyboard::Pause: return "CAPS"; break;
		}

		return "error";
	}
}

// KeyEvent struct default constructor doesn't initialize variables, as far as I can tell, anyway...
struct SfKeyEvent : public sf::Event::KeyEvent
{
	SfKeyEvent(const sf::Keyboard::Key _code = sf::Keyboard::Unknown,
		const bool _alt = false, const bool _control = false, const bool _shift = false, const bool _system = false)
	{
		code = _code;
		alt = _alt;
		control = _control;
		shift = _shift;
		system = _system;
	}

	SfKeyEvent(const sf::Event::KeyEvent& cpy)
	{
		code = cpy.code;
		alt = cpy.alt;
		control = cpy.control;
		shift = cpy.shift;
		system = cpy.system;
	}

	virtual ~SfKeyEvent() {}
	void clear() { *this = SfKeyEvent(); }
};

// The sound buffer we use must not be destroyed as long as the sprite is still being used
class SfSoundSafe : public sf::Sound
{
	public:
		SfSoundSafe(shared_ptr<sf::SoundBuffer> b) :
			sbuffer(b),
			m_originalVol(0)
		{
			setBuffer(*b);
		}
		
		virtual ~SfSoundSafe() {}

		void rememberScreenPosition(const sf::Vector2f& screenPos) { m_screenPos = screenPos; }
		void setScreenPosition(const sf::Vector2f& screenPos);
		void rememberVolume(const float v) { m_originalVol = v; }
		
		void popScreenPos() { setScreenPosition(m_screenPos); }
		float getRememberedVolume() { return m_originalVol; }

	private:
		float m_originalVol;
		sf::Vector2f m_screenPos;
		shared_ptr<sf::SoundBuffer> sbuffer;
};