#include "DebugManager.h"
#include <imgui-SFML.h>
#include <imgui.h>
#include "../Config/Config.h"
#include "../SceneManager/SceneManager.h"
#include "../CameraManager/CameraManager.h"
#include "../Time/Time.h"
#include "../Performance/CPU.h"
#include "../Config/Config.h"

void DebugManager::Update(const sf::Window& window) {
    static bool lastF1 = false;
    bool nowF1 = InputManager::Instance().GetKeyInput().GetKey(sf::Keyboard::Key::F1);
    //bool nowF1 = InputManager::Instance().padInput->IsPressedPad(0,0);
    if (nowF1 && !lastF1) debugMode = !debugMode;
    // 通常の時は画面サイズを元に戻す
    if (!debugMode)
    {
        CameraManager::Instance().SetZoomLevel(1);
    }
    lastF1 = nowF1;

    // メモリ使用率のUpdate
    memory.Update();
}

void DebugManager::Render(const sf::Texture* renderTexture) {
    ImGuiViewport* id = ImGui::GetMainViewport();
    ImGui::DockSpaceOverViewport(id->ID);
	// ゲーム画面表示
    this->RenderGameScreen(renderTexture);
	// シーン管理
	this->RenderSceneManagement();
	// ログ出力ウィンドウ
	this->RenderLogWindow();
	// カメラコントロール
	this->RenderCameraControl();
    // Timeクラス関係のログ表示
    this->RenderPerformance();
    // スクリーン上のGui表示
    SceneManager::Instance().RenderImGui(renderTexture);
    // 言語設定
    this->RenderLanguageSettings();
    // Input
    InputManager::Instance().RenderImGui();
}

void DebugManager::RenderGameScreen(const sf::Texture* renderTexture) {
    // ゲーム画面表示
    if (renderTexture) {
        DebugGui::Begin("Debug Controls", "デバックコントローラー");
        DebugGui::Text("Game scene Render", "ゲームスクリーン");

        // 反転処理
        // 一時的なスプライト用意する
        sf::Sprite tempSprite(*renderTexture);
        // サイズ取得
        sf::Vector2u texSize = renderTexture->getSize();
        // テクスチャ矩形を上下反転させてImGui上で元の状態に見せる
        tempSprite.setTextureRect(sf::IntRect(
            // 左上の座標。Y はテクスチャの下端から開始
            { 0, static_cast<int>(texSize.y) },
            // y軸だけ反転する
            { static_cast<int>(texSize.x), -static_cast<int>(texSize.y) }
        ));
        ImGui::Image(tempSprite, sf::Vector2f(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f));

        // メモ
        // ImGui::Image(*renderTexture, sf::Vector2f(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f));
        // なぜかImGui上ではy軸が反転してる
        // 可能性として
        // ImGui::Image関数のUV座標指定が逆になっている
        // sf::Texture自体が上下反転している
        // 上はApplicationでtextureをSpriteで描画しても上下反転しないので、
        // ImGui側の問題の可能性が高い

        //ImGui::Image(target,
        //sf::Vector2f(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f),
        //sf::Color::White,
        //sf::Color::Transparent
        //);

        //mGui::Image(*renderTexture, ImVec2::toImVec2(size), ImVec2(0, 0), ImVec2(1, 1), toImColor(tintColor), toImColor(borderColor));
        //ImGui::Image(*renderTexture, sf::Vector2f(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f), ImVec2(0, 0), ImVec2(1, 1), sf::Color::White, sf::Color::Transparent);
        //ImGui::Image(renderTexture->getNativeHandle(), sf::Vector2f(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f), ImVec2(0, 0), ImVec2(1, 1), );
        //void Image(const sf::Texture & texture, const sf::Vector2f & size, const sf::Color & tintColor, const sf::Color & borderColor)
        //{
        //    ImTextureID textureID = convertGLTextureHandleToImTextureID(texture.getNativeHandle());
        //
        //    ImGui::Image(textureID, toImVec2(size), ImVec2(0, 0), ImVec2(1, 1), toImColor(tintColor), toImColor(borderColor));
        //}
//        ImGui::Image(*renderTexture, sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
        DebugGui::End(); // Debug Controls
    }
}

void DebugManager::RenderSceneManagement() {
    // シーン切り替えコントロール
    DebugGui::Begin("Scene Management", "シーンマネジメント");
    ImGui::Separator();

    SceneManager& screenManager = SceneManager::Instance();
    const std::string& currentScreen = screenManager.GetCurrentSceneName();
    const auto& registeredScreens = screenManager.GetRegisteredScenes();

    // コンボボックスに表示するアイテムリストを作成
    std::vector<std::string> screenNames;
    for (const auto& pair : registeredScreens) {
        screenNames.push_back(pair.first);
    }

    // ImGui::Comboで利用するためにstd::vector<const char*>に変換
    std::vector<const char*> screenNames_c_str;
    for (const auto& name : screenNames) {
        screenNames_c_str.push_back(name.c_str());
    }

    static int currentItem = 0; // 現在選択されているインデックス

    // 現在のシーン名を見つけ、currentItemを初期化
    if (screenNames.size() > 0) {
        auto it = std::find(screenNames.begin(), screenNames.end(), currentScreen);
        if (it != screenNames.end()) {
            currentItem = std::distance(screenNames.begin(), it);
        }
    }

    DebugGui::Text("Current Scene: %s", "現在のシーン: %s", currentScreen.c_str());

    if (ImGui::Combo("##ScreenSelector", &currentItem, screenNames_c_str.data(), screenNames_c_str.size())) {
        // 選択が変更された場合
        const char* selectedName = screenNames_c_str[currentItem];

        // **シーン切り替えの実行**
        if (selectedName != currentScreen) {
            screenManager.ChangeScene(selectedName);
        }
    }

    DebugGui::End(); // Scene Management
}

void DebugManager::RenderLogWindow() {
    // ログ画面 (Placeholder)
    DebugGui::Begin("Log Window", "ログウィンドウ");
    ImGui::Separator();
    // TODO:ログ後で作ろうね
    // どんなふうに作ろうかな
    DebugGui::End(); // Log Window
}

void DebugManager::RenderCameraControl() {
    // カメラズーム
    DebugGui::Begin("CameraSetting", "カメラ設定");
    ImGui::Separator();

    // シーンに依存しないカメラマネージャーのインスタンスを取得
    CameraManager& cameraManager = CameraManager::Instance();

    // 現在のズームレベルを取得して初期値とする
    // 現在の状態をずっと維持？しときたいからstatic消す
    float zoomLevel = cameraManager.GetZoomLevel();
    //static float zoomLevel = cameraManager.GetZoomLevel();

    if (DebugGui::SliderFloat("Zoom Level", "ズーム倍率", &zoomLevel, 0.1f, 5.0f, " % .1f", ImGuiSliderFlags_AlwaysClamp)) {
        // スライダーが操作されたら CameraManager にズームレベルを適用
        cameraManager.SetZoomLevel(zoomLevel);
    }
    DebugGui::Text("Range: 0.1 (far) ~ 5.0 (near)", "幅: 0.1 (遠い) ~ 5.0 (近い)");

    DebugGui::End(); // Camera Setting
}

void DebugManager::RenderPerformance() {
    // TODO: あとでやる気がでたら綺麗にする
    DebugGui::Begin("Performance", "プレファレンス");
    // FPSの描画
    Time::Instance().RenderImGui();
    
    DebugGui::SeparatorText("System", "システム");
    {   // ImGuiのChildで囲われているのでわかりやすくスコープを付けておく
        ImGui::BeginChild("PerfCard", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
        // CPU 描画
        CpuUsage::Instance().RenderImGui();
        // メモリ描画
        memory.RenderImGui();
        ImGui::EndChild();
    }
    DebugGui::End();
}

// TODO: 実装 今は必要ないけど後々使うかもだからおいておく
void DebugManager::RenderOtherDebugInfo() {
}

void DebugManager::RenderLanguageSettings()
{
    DebugGui::Begin("Settings", "設定");

    // 言語切り替えラジオボタン
    // 現在の状態を取得して判定
    if (DebugGui::RadioButton("English", Language::Get() == Language::Type::English)) {
        Language::Set(Language::Type::English);
    }

    ImGui::SameLine();
    if (DebugGui::RadioButton("日本語", Language::Get() == Language::Type::Japanese)) {
        Language::Set(Language::Type::Japanese);
    }

    DebugGui::End();
}