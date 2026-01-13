#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "../Singleton/Singleton.h"
#include "../Config/Config.h"

class CameraManager : public Singleton<CameraManager> {
    friend class Singleton<CameraManager>;
protected:
    CameraManager();
private:
    sf::View view;
public:
    // カメラ操作インターフェース
    void SetZoomLevel(float zoom);
    float GetZoomLevel() const;
    // 現在のViewを取得
    const sf::View& GetCurrentView() const { return view; }
    // Viewの初期化やリセット
    void ResetView();
    // カメラを中央にセット
	void SetCenter(const sf::Vector2f& center);
};