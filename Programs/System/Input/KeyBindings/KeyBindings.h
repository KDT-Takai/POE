#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <array>
#include <string>
#include "../../Singleton/Singleton.h"

// Rebindable gameplay actions. Menu navigation itself (Up/Down/Left/Right/Enter/
// Backspace/X/Delete) stays on fixed keys shared across every UI system and is
// intentionally not included here (see AI/DECISIONS.md).
enum class GameAction {
    MoveUp, MoveDown, MoveLeft, MoveRight,
    Roll,
    Skill1, Skill2, Skill3, Skill4, Skill5,
    ToggleCharacterSheet, ToggleInventory, TogglePassiveTree, ToggleSkillGems,
    VendorReroll,
    Count
};

// Singleton holding the key bound to each action, persisted to disk. Every
// system should read a key via KeyBindings::Instance().Get(GameAction::X)
// instead of hardcoding an sf::Keyboard::Key literal.
class KeyBindings : public Singleton<KeyBindings> {
    friend class Singleton<KeyBindings>;
protected:
    KeyBindings();

public:
    sf::Keyboard::Key Get(GameAction action) const {
        return m_bindings[static_cast<size_t>(action)];
    }
    void Set(GameAction action, sf::Keyboard::Key key) {
        m_bindings[static_cast<size_t>(action)] = key;
    }

    // If key is already bound to a different action, returns true and sets outConflict to it.
    bool FindConflict(sf::Keyboard::Key key, GameAction ignoring, GameAction& outConflict) const;

    void ResetToDefaults();
    static std::string ActionLabel(GameAction action); // for UI display
    static std::string ActionKey(GameAction action);   // stable identifier for the config file

    void SaveToDisk(const std::string& path = "keybinds.cfg") const;
    bool LoadFromDisk(const std::string& path = "keybinds.cfg");

private:
    std::array<sf::Keyboard::Key, static_cast<size_t>(GameAction::Count)> m_bindings;
};
