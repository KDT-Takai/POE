// --------------------------------
// Application.h
// --------------------------------
#pragma once
// --------------------------------
// SFML - Simple and Fast Multimedia Library
// --------------------------------
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
// --------------------------------
// Headers
// --------------------------------
#include <iostream>
#include "../DebugManager/DebugManager.h"

// Application class that manages the SFML window
class Application {
public:
	Application();
	~Application();
	// Main application loop
	void run();
private:
	// Main SFML window ウィンドウ
	std::unique_ptr<sf::RenderWindow> window;
	// Clock to track time between frames フレーム間の時間を追跡するクロック
	sf::Clock deltaClock;
	// Render texture for off-screen rendering オフスクリーンレンダリング用のレンダーテクスチャ
	std::unique_ptr<sf::RenderTexture> renderTexture;
	// Sprite to display the render texture レンダーテクスチャを表示するスプライト
	std::unique_ptr<sf::Sprite> renderSprite;
	// Debug manager for handling debug features デバッグ機能を管理するデバッグマネージャー
	// シングルトンに変更したため不要
//	std::unique_ptr<DebugManager> debugManager;
	
	// Initialize the application イベントの処理
	void ProcessEvents();
	// Update the application state アプリケーションの更新状態
	void Update(sf::Time deltaTime);
	// Render the current frame レンダリング現在のフレーム
	void Render();

	// Scene management
//	std::unique_ptr<ScreenManager> screenManager;
};