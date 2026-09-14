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
    sf::Vector2f baseCenter;
    float shakeTimer = 0.0f;
    float shakeIntensity = 0.0f;
    void ApplyCenterWithShake();
public:
    // �J��������C���^�[�t�F�[�X
    void SetZoomLevel(float zoom);
    float GetZoomLevel() const;
    // ���݂�View���擾
    const sf::View& GetCurrentView() const { return view; }
    // View�̏������⃊�Z�b�g
    void ResetView();
    // �J�����𒆉��ɃZ�b�g
	void SetCenter(const sf::Vector2f& center);

    void Shake(float intensity, float duration);
    void UpdateShake(float dt);
};