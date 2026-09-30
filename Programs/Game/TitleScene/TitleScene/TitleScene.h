#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <random>
#include <System/SceneManager/SceneBase.h>

class TitleScene : public SceneBase {
public:
    static const char* GetName() { return "TitleScene"; }

    TitleScene();

    void Update() override;
    void Render(sf::RenderTarget& target) override;
    void RenderImGui(const sf::Texture* renderTexture) override;

private:
    struct Ember {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float life;
        float maxLife;
        float size;
        float phase;
    };

    void SpawnEmber(bool randomHeight);
    void DrawCenteredText(sf::RenderTarget& target, const std::string& utf8, unsigned int size,
        float y, sf::Color fill, float letterSpacing = 1.0f) const;

    std::unique_ptr<sf::Sprite> m_background;
    std::shared_ptr<sf::Font> m_font;
    std::vector<Ember> m_embers;
    std::mt19937 m_rng{ std::random_device{}() };
    float m_elapsed = 0.0f;
    bool m_hardcoreSelected = false;
};
