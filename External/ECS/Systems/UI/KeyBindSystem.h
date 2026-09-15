#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <array>
#include "../../Registry/Registry.h"
#include "System/Resource/ResourceManager/ResourceManager.h"
#include "System/Input/InputManager.h"
#include "System/Input/InputUtils/InputUtils.h"
#include "System/Input/KeyBindings/KeyBindings.h"

// Options panel (O key) for rebinding the gameplay actions listed in KeyBindings.
// Menu navigation itself (Up/Down/Enter/Escape here) stays on fixed keys by design;
// only the actions in GameAction are remappable (see AI/DECISIONS.md).
class KeyBindSystem {
private:
    std::shared_ptr<sf::Font> m_font;
    int m_selectedIndex = 0;
    bool m_awaitingKey = false;

    static constexpr int kActionCount = static_cast<int>(GameAction::Count);
    static constexpr int kResetRowIndex = kActionCount; // virtual "Reset to Defaults" row

public:
    bool isOpen = false;
    std::string lastActionMessage;
    float messageTimer = 0.0f;

    KeyBindSystem() {
        m_font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
    }

    void Toggle() { isOpen = !isOpen; if (!isOpen) m_awaitingKey = false; }
    void Close() { isOpen = false; m_awaitingKey = false; }
    bool IsAwaitingKey() const { return m_awaitingKey; }

    void Update(Registry& registry, float dt) {
        if (messageTimer > 0.0f) messageTimer -= dt;
        if (!isOpen) return;

        auto& keyInput = InputManager::Instance().GetKeyInput();
        auto& binds = KeyBindings::Instance();

        if (m_awaitingKey) {
            if (keyInput.IsGetKey(sf::Keyboard::Key::Escape)) {
                m_awaitingKey = false;
                lastActionMessage = "Cancelled";
                messageTimer = 1.5f;
                return;
            }

            sf::Keyboard::Key pressed = sf::Keyboard::Key::Unknown;
            for (int i = 0; i < static_cast<int>(sf::Keyboard::KeyCount); ++i) {
                auto key = static_cast<sf::Keyboard::Key>(i);
                if (keyInput.IsGetKey(key)) { pressed = key; break; }
            }
            if (pressed == sf::Keyboard::Key::Unknown) return;

            if (IsReserved(pressed)) {
                lastActionMessage = "That key is reserved for menu navigation";
                messageTimer = 2.0f;
                return;
            }

            auto action = static_cast<GameAction>(m_selectedIndex);
            GameAction conflict;
            if (binds.FindConflict(pressed, action, conflict)) {
                lastActionMessage = "Already used by: " + KeyBindings::ActionLabel(conflict);
                messageTimer = 2.0f;
                return;
            }

            binds.Set(action, pressed);
            binds.SaveToDisk();
            lastActionMessage = KeyBindings::ActionLabel(action) + " -> " + KeyToString(pressed);
            messageTimer = 2.0f;
            m_awaitingKey = false;
            return;
        }

        if (keyInput.IsGetKey(sf::Keyboard::Key::Down)) m_selectedIndex = (m_selectedIndex + 1) % (kActionCount + 1);
        if (keyInput.IsGetKey(sf::Keyboard::Key::Up)) m_selectedIndex = (m_selectedIndex - 1 + kActionCount + 1) % (kActionCount + 1);

        if (keyInput.IsGetKey(sf::Keyboard::Key::Enter)) {
            if (m_selectedIndex == kResetRowIndex) {
                binds.ResetToDefaults();
                binds.SaveToDisk();
                lastActionMessage = "Reset all bindings to defaults";
                messageTimer = 2.0f;
            } else {
                m_awaitingKey = true;
                lastActionMessage = "Press a key... (Escape to cancel)";
                messageTimer = 5.0f;
            }
        }
    }

    void Render(Registry& registry, sf::RenderTarget& target) {
        if (!isOpen || !m_font) return;

        auto& binds = KeyBindings::Instance();

        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());

        sf::Vector2u winSize = target.getSize();
        float panelW = 520.0f;
        float panelH = 520.0f;
        float panelX = (winSize.x - panelW) / 2.0f;
        float panelY = (winSize.y - panelH) / 2.0f;

        sf::RectangleShape bg({ panelW, panelH });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 15, 20, 235));
        bg.setOutlineColor(sf::Color(150, 150, 160));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);

        DrawText(target, panelX + 20.0f, panelY + 15.0f,
            "Key Bindings (O to close) - Up/Down select, Enter rebind", 15, sf::Color(255, 220, 120));

        float rowY = panelY + 50.0f;
        float rowHeight = 22.0f;

        for (int i = 0; i < kActionCount; ++i) {
            auto action = static_cast<GameAction>(i);
            float y = rowY + static_cast<float>(i) * rowHeight;
            bool selected = (i == m_selectedIndex);

            if (selected) {
                sf::RectangleShape highlight({ panelW - 40.0f, rowHeight });
                highlight.setPosition({ panelX + 20.0f, y });
                highlight.setFillColor(sf::Color(60, 60, 90, 180));
                target.draw(highlight);
            }

            std::string keyLabel = (selected && m_awaitingKey) ? "..." : KeyToString(binds.Get(action));
            std::string line = KeyBindings::ActionLabel(action) + ": " + keyLabel;
            DrawText(target, panelX + 26.0f, y + 3.0f, line, 14, sf::Color(220, 220, 220));
        }

        {
            float y = rowY + static_cast<float>(kActionCount) * rowHeight + 10.0f;
            bool selected = (m_selectedIndex == kResetRowIndex);
            if (selected) {
                sf::RectangleShape highlight({ panelW - 40.0f, rowHeight });
                highlight.setPosition({ panelX + 20.0f, y });
                highlight.setFillColor(sf::Color(90, 60, 60, 180));
                target.draw(highlight);
            }
            DrawText(target, panelX + 26.0f, y + 3.0f, "Reset to Defaults", 14, sf::Color(255, 180, 160));
        }

        if (messageTimer > 0.0f && !lastActionMessage.empty()) {
            DrawText(target, panelX + 20.0f, panelY + panelH - 24.0f, lastActionMessage, 13, sf::Color(255, 230, 120));
        }

        target.setView(oldView);
    }

private:
    bool IsReserved(sf::Keyboard::Key key) const {
        // O itself is the fixed (non-rebindable) key that opens/closes this panel
        // (see GameScene::Update); reserve it too so a rebind can't collide with it.
        static const std::array<sf::Keyboard::Key, 9> kReserved = {
            sf::Keyboard::Key::Up, sf::Keyboard::Key::Down, sf::Keyboard::Key::Left, sf::Keyboard::Key::Right,
            sf::Keyboard::Key::Enter, sf::Keyboard::Key::Backspace, sf::Keyboard::Key::X, sf::Keyboard::Key::Delete,
            sf::Keyboard::Key::O
        };
        for (auto reserved : kReserved) {
            if (reserved == key) return true;
        }
        return false;
    }

    void DrawText(sf::RenderTarget& target, float x, float y, const std::string& str, unsigned int size, sf::Color color) {
        sf::Text text(*m_font, str, size);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition({ x, y });
        target.draw(text);
    }
};
