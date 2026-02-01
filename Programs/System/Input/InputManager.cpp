#include "InputManager.h"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include "../CameraManager/CameraManager.h"

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

sf::Vector2f InputManager::GetMouseWorldPosition() const {
	if (!m_window) return sf::Vector2f(0.0f, 0.0f);

	sf::Vector2i pixelPos = sf::Mouse::getPosition(*m_window);

	// ‚±‚±‚È‚ç CameraManager ‚Ì³‘Ì‚ª‚í‚©‚Á‚Ä‚¢‚é‚Ì‚ÅƒGƒ‰[‚É‚È‚ç‚È‚¢
	auto& view = CameraManager::Instance().GetCurrentView();

	return m_window->mapPixelToCoords(pixelPos, view);
}