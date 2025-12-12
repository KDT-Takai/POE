#pragma once
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <string>

// SFMLÇÃsf::Keyboard::Key Å® ï∂éöóÒïœä∑
inline std::string KeyToString(sf::Keyboard::Key key) {
    switch (key) {
    case sf::Keyboard::Key::A: return "A";
    case sf::Keyboard::Key::B: return "B";
    case sf::Keyboard::Key::C: return "C";
    case sf::Keyboard::Key::D: return "D";
    case sf::Keyboard::Key::E: return "E";
    case sf::Keyboard::Key::F: return "F";
    case sf::Keyboard::Key::G: return "G";
    case sf::Keyboard::Key::H: return "H";
    case sf::Keyboard::Key::I: return "I";
    case sf::Keyboard::Key::J: return "J";
    case sf::Keyboard::Key::K: return "K";
    case sf::Keyboard::Key::L: return "L";
    case sf::Keyboard::Key::M: return "M";
    case sf::Keyboard::Key::N: return "N";
    case sf::Keyboard::Key::O: return "O";
    case sf::Keyboard::Key::P: return "P";
    case sf::Keyboard::Key::Q: return "Q";
    case sf::Keyboard::Key::R: return "R";
    case sf::Keyboard::Key::S: return "S";
    case sf::Keyboard::Key::T: return "T";
    case sf::Keyboard::Key::U: return "U";
    case sf::Keyboard::Key::V: return "V";
    case sf::Keyboard::Key::W: return "W";
    case sf::Keyboard::Key::X: return "X";
    case sf::Keyboard::Key::Y: return "Y";
    case sf::Keyboard::Key::Z: return "Z";

    case sf::Keyboard::Key::Num0: return "0";
    case sf::Keyboard::Key::Num1: return "1";
    case sf::Keyboard::Key::Num2: return "2";
    case sf::Keyboard::Key::Num3: return "3";
    case sf::Keyboard::Key::Num4: return "4";
    case sf::Keyboard::Key::Num5: return "5";
    case sf::Keyboard::Key::Num6: return "6";
    case sf::Keyboard::Key::Num7: return "7";
    case sf::Keyboard::Key::Num8: return "8";
    case sf::Keyboard::Key::Num9: return "9";

    case sf::Keyboard::Key::Space: return "Space";
    case sf::Keyboard::Key::Enter: return "Enter";
    case sf::Keyboard::Key::Escape: return "Escape";
    case sf::Keyboard::Key::LShift: return "LShift";
    case sf::Keyboard::Key::RShift: return "RShift";
    case sf::Keyboard::Key::LControl: return "LControl";
    case sf::Keyboard::Key::RControl: return "RControl";
    case sf::Keyboard::Key::Tab: return "Tab";
    case sf::Keyboard::Key::Backspace: return "Backspace";
    case sf::Keyboard::Key::Left: return "Left";
    case sf::Keyboard::Key::Right: return "Right";
    case sf::Keyboard::Key::Up: return "Up";
    case sf::Keyboard::Key::Down: return "Down";

    case sf::Keyboard::Key::F1: return "F1";
    case sf::Keyboard::Key::F2: return "F2";
    case sf::Keyboard::Key::F3: return "F3";
    case sf::Keyboard::Key::F4: return "F4";
    case sf::Keyboard::Key::F5: return "F5";
    case sf::Keyboard::Key::F6: return "F6";
    case sf::Keyboard::Key::F7: return "F7";
    case sf::Keyboard::Key::F8: return "F8";
    case sf::Keyboard::Key::F9: return "F9";
    case sf::Keyboard::Key::F10: return "F10";
    case sf::Keyboard::Key::F11: return "F11";
    case sf::Keyboard::Key::F12: return "F12";

    default: return "Unknown";
    }
}

inline std::string MouseButtonToString(sf::Mouse::Button button)
{
    switch (button) {
    case sf::Mouse::Button::Left:     return "Left";
    case sf::Mouse::Button::Right:    return "Right";
    case sf::Mouse::Button::Middle:   return "Middle";
    case sf::Mouse::Button::Extra1:   return "Extra1";
    case sf::Mouse::Button::Extra2:   return "Extra2";
    default:                  return "Unknown";
    }
}