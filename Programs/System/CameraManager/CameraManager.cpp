#include "CameraManager.h"

CameraManager::CameraManager() {
    // コンストラクタでViewを初期化
    ResetView();
}

void CameraManager::ResetView() {
    // Config.h の定数を使用して初期サイズを設定
    view.setSize({ WINDOW_WIDTH, WINDOW_HEIGHT });
    view.setCenter({ WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f });
}

void CameraManager::SetZoomLevel(float zoom) {
    // ズームレベルに基づいてViewのサイズを再計算
    float ratio = 1.0f / zoom;
    float newWidth = WINDOW_WIDTH * ratio;
    float newHeight = WINDOW_HEIGHT * ratio;

    view.setSize({newWidth, newHeight});
}

float CameraManager::GetZoomLevel() const {
    // ズームレベルを逆算して返す
    return WINDOW_WIDTH / view.getSize().x;
}