#pragma once
#include <SFML/Graphics.hpp>
#include "../Input/InputManager.h"
#include "../Performance/MemoryMonitor.h"

class DebugManager {
public:
    void Update(const sf::Window& window);
    bool IsDebugMode() const { return debugMode; }
    void Render(const sf::Texture* renderTexture);
private:
    // ゲーム画面の表示
	void RenderGameScreen(const sf::Texture* renderTexture);
	// シーン管理画面の表示
	void RenderSceneManagement();
	// ログ画面の表示
	void RenderLogWindow();
	// カメラコントロール画面の表示
	void RenderCameraControl();
	// オブジェクト設定画面の表示
	void RenderObjectSettings();
	// Timeクラス関係のログ表示
	void RenderPerformance();
	// その他デバッグ情報画面の表示
	void RenderOtherDebugInfo();

    // デバックモード false:not debug mode true : debug mode
    bool debugMode = false;

	// メモリーの使用率の描画
	MemoryMonitor memory;
};