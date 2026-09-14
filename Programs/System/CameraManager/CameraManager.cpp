#include "CameraManager.h"

CameraManager::CameraManager() {
    // �R���X�g���N�^��View��������
    ResetView();
}

void CameraManager::ResetView() {
    // Config.h �̒萔���g�p���ď����T�C�Y��ݒ�
    view.setSize({ WINDOW_WIDTH, WINDOW_HEIGHT });
    view.setCenter({ WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f });
}

void CameraManager::SetCenter(const sf::Vector2f& center) {
    baseCenter = center;
    ApplyCenterWithShake();
}

void CameraManager::Shake(float intensity, float duration) {
    shakeIntensity = intensity;
    shakeTimer = duration;
}

void CameraManager::UpdateShake(float dt) {
    if (shakeTimer > 0.0f) {
        shakeTimer -= dt;
        if (shakeTimer < 0.0f) shakeTimer = 0.0f;
        ApplyCenterWithShake();
    }
}

void CameraManager::ApplyCenterWithShake() {
    sf::Vector2f offset(0.0f, 0.0f);
    if (shakeTimer > 0.0f) {
        offset.x = (static_cast<float>(rand() % 2000) / 1000.0f - 1.0f) * shakeIntensity;
        offset.y = (static_cast<float>(rand() % 2000) / 1000.0f - 1.0f) * shakeIntensity;
    }
    view.setCenter(baseCenter + offset);
}

void CameraManager::SetZoomLevel(float zoom) {
    // �Y�[�����x���Ɋ�Â���View�̃T�C�Y���Čv�Z
    float ratio = 1.0f / zoom;
    float newWidth = WINDOW_WIDTH * ratio;
    float newHeight = WINDOW_HEIGHT * ratio;

    view.setSize({newWidth, newHeight});
}

float CameraManager::GetZoomLevel() const {
    // �Y�[�����x�����t�Z���ĕԂ�
    return WINDOW_WIDTH / view.getSize().x;
}