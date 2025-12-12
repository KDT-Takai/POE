#pragma once
#include <array>
#include <SFML/Graphics.hpp>	// sf::Vector2i
#include <SFML/Window/Mouse.hpp>

class MouseInput {
public:
	MouseInput();
	// マウスの状態を更新
	void Update(const sf::RenderWindow& window);
	// マウス入力関係
	bool IsGetMouse(sf::Mouse::Button button) const;		// 押した瞬間
	bool GetMouse(sf::Mouse::Button button) const;			// 押されている間
	bool GetMouseRepeat(sf::Mouse::Button button) const;	// 長押し
	sf::Vector2i GetMousePoint();							// マウスの座標取得

	// Dedub用のログ出力
	void RenderImGui();

private:
	static constexpr int MOUSE_BUTTON_MAX = sf::Mouse::ButtonCount;	// マウスボタンの最大数
	std::array<bool, MOUSE_BUTTON_MAX> nowMouseInput;				// 現在のマウス入力状態
	std::array<bool, MOUSE_BUTTON_MAX> beforMouseInput;				// 前回のマウス入力状態
	int mouseX, mouseY;												// マウスのX座標、Y座標
};