#include "Application.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include "../Input/InputManager.h"
#include "../Config/Config.h"
#include "../SceneManager/SceneManager.h"
#include "../CameraManager/CameraManager.h"
#include "../Time/Time.h"
#include "../Resource/ResourceManager/ResourceManager.h"

Application::Application() {
	// Create the main window ウィンドウの作成
	window = std::make_unique<sf::RenderWindow>(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), WINDOW_TITLE);
	// Create the render texture レンダーテクスチャの作成
	renderTexture = std::make_unique<sf::RenderTexture>();
	// Create the debug manager デバッグマネージャーの作成
//	debugManager = std::make_unique<DebugManager>();

	// Window settings ウィンドウの設定
	window->setFramerateLimit(FRAMERATE_LIMIT);
	// ImGui initialization ImGuiの初期化
	ImGui::SFML::Init(*window);
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
//	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::StyleColorsClassic();
	// RenderTexture settings レンダーテクスチャの設定
	renderTexture->resize(sf::Vector2u{WINDOW_WIDTH, WINDOW_HEIGHT});

	// テクスチャをセット（ポインタ/参照を保持するだけなので軽量）
	renderSprite = std::make_unique<sf::Sprite>((renderTexture->getTexture()));

	Time::Instance().SetTargetFPS(60);
	window->setFramerateLimit(60);

	// リソース自動読み込み
	// フォントは自動読み込みする必要はないので今後自動読み込みではなく手動読み込みに変更する可能性あり
	ResourceManager::Instance().loadAllTextures("Assets/Textures");
	ResourceManager::Instance().loadAllFonts("Assets/Fonts");
	ResourceManager::Instance().loadAllSounds("Assets/Sounds");

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();
	const char* fontPath = "Assets/Fonts/NotoSansJP-Regular.ttf";
	ImFontConfig config;
	config.FontNo = 0;
	float fontSize = 17.0f;
	ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, fontSize, &config, io.Fonts->GetGlyphRangesJapanese());
	if (font == nullptr)
	{
		throw "error";
	}
	ImGui::SFML::UpdateFontTexture();
}

Application::~Application() {
	ImGui::SFML::Shutdown();
}

void Application::run() {
	while(window->isOpen()) {
		ProcessEvents();
		sf::Time deltaTime = deltaClock.restart();
		ImGui::SFML::Update(*window, deltaTime);
		Time::Instance().Update();
		InputManager::Instance().Update(*window);
		Update(deltaTime);
		Render();
	}
}

void Application::ProcessEvents() {
	while (auto event = window->pollEvent()) {
		ImGui::SFML::ProcessEvent(*window, *event);
		// Close window: exit ウィンドウを閉じる：終了
		if (event->is<sf::Event::Closed>()) {
			window->close();
		}
	}
}

void Application::Update(sf::Time deltaTime) {
	// Spriteにテクスチャを更新
	renderSprite->setTexture(renderTexture->getTexture(), true);
#ifdef _DEBUG
	DebugManager::Instance().Update(*window);
#endif
	SceneManager::Instance().Update();
// ゲームを停止させる場合
//	if (!debugMode_flag)
//		screenManager->Update(dt.asSeconds());
}

void Application::Render() {

	renderTexture->clear(sf::Color(128,128,128));

	SceneManager::Instance().Render(*renderTexture);

//	static sf::CircleShape circle2(50.0f);
//	circle2.setFillColor(sf::Color::Green);
//	circle2.setPosition({200,200});
	
	// renderTexture->getTexture()の場合これは画像として描画してる
//	static sf::Sprite sprite(renderTexture->getTexture());
//	sprite.setScale({0.5f, 0.5f});

	renderTexture->display();

	window->clear();
#ifdef _DEBUG
	if (!DebugManager::Instance().IsDebugMode()) {
		window->draw(*renderSprite);
	}
	if (DebugManager::Instance().IsDebugMode()){
		DebugManager::Instance().Render(&renderTexture->getTexture());
	}
#else
	window->draw(*renderSprite);
#endif

//	window->draw(sf::Sprite(renderTexture->getTexture()));
//	window->draw(circle2);
//	window->draw(sprite);

	ImGui::SFML::Render(*window);

	window->display();
}