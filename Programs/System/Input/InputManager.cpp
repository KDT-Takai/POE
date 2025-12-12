#include "InputManager.h"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

InputManager::InputManager()
{
	keyInput = std::make_unique<KeyInput>();
	mouseInput = std::make_unique<MouseInput>();
	padInput = std::make_unique<PadInput>();
}

void InputManager::Update(const sf::RenderWindow& window)
{
	keyInput->Update();
	mouseInput->Update(window);
	padInput->Update();
}

void InputManager::RenderImGui()
{
	keyInput->RenderImGui();
	mouseInput->RenderImGui();
	padInput->RenderImGui();
}