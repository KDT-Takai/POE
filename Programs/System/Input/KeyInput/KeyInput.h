#pragma once
#include <array>
#include <SFML/Window/Keyboard.hpp>

// キーボード入力管理クラス
class KeyInput {
public:
	KeyInput();
	// キーの状態を更新
	void Update();
	// キー入力関係
	bool IsGetKey(sf::Keyboard::Key key) const;     // 押した瞬間
	bool GetKey(sf::Keyboard::Key key) const;       // 押されている間
	bool GetKeyRepeat(sf::Keyboard::Key key) const; // 長押し
	
	// Dedub用のログ出力
	void RenderImGui();

private:
	static constexpr int KEY_MAX = sf::Keyboard::KeyCount;	// キーの最大数
	std::array<bool, KEY_MAX> nowKeyInput;					// 現在のキー入力状態
	std::array<bool, KEY_MAX> beforKeyInput;				// 前回のキー入力状態
};