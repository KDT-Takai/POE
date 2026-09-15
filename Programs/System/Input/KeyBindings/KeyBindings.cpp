#include "KeyBindings.h"
#include "../InputUtils/InputUtils.h"
#include <fstream>
#include <unordered_map>

KeyBindings::KeyBindings() {
    ResetToDefaults();
    LoadFromDisk(); // missing/corrupt file just leaves the defaults in place
}

void KeyBindings::ResetToDefaults() {
    m_bindings[static_cast<size_t>(GameAction::MoveUp)] = sf::Keyboard::Key::W;
    m_bindings[static_cast<size_t>(GameAction::MoveDown)] = sf::Keyboard::Key::S;
    m_bindings[static_cast<size_t>(GameAction::MoveLeft)] = sf::Keyboard::Key::A;
    m_bindings[static_cast<size_t>(GameAction::MoveRight)] = sf::Keyboard::Key::D;
    m_bindings[static_cast<size_t>(GameAction::Roll)] = sf::Keyboard::Key::Space;
    m_bindings[static_cast<size_t>(GameAction::Skill1)] = sf::Keyboard::Key::E;
    m_bindings[static_cast<size_t>(GameAction::Skill2)] = sf::Keyboard::Key::Q;
    m_bindings[static_cast<size_t>(GameAction::Skill3)] = sf::Keyboard::Key::R;
    m_bindings[static_cast<size_t>(GameAction::Skill4)] = sf::Keyboard::Key::V;
    m_bindings[static_cast<size_t>(GameAction::Skill5)] = sf::Keyboard::Key::F;
    m_bindings[static_cast<size_t>(GameAction::ToggleCharacterSheet)] = sf::Keyboard::Key::C;
    m_bindings[static_cast<size_t>(GameAction::ToggleInventory)] = sf::Keyboard::Key::I;
    m_bindings[static_cast<size_t>(GameAction::TogglePassiveTree)] = sf::Keyboard::Key::P;
    m_bindings[static_cast<size_t>(GameAction::ToggleSkillGems)] = sf::Keyboard::Key::K;
    m_bindings[static_cast<size_t>(GameAction::VendorToggle)] = sf::Keyboard::Key::B;
    m_bindings[static_cast<size_t>(GameAction::VendorReroll)] = sf::Keyboard::Key::T;
}

bool KeyBindings::FindConflict(sf::Keyboard::Key key, GameAction ignoring, GameAction& outConflict) const {
    for (size_t i = 0; i < m_bindings.size(); ++i) {
        auto action = static_cast<GameAction>(i);
        if (action == ignoring) continue;
        if (m_bindings[i] == key) {
            outConflict = action;
            return true;
        }
    }
    return false;
}

std::string KeyBindings::ActionLabel(GameAction action) {
    switch (action) {
    case GameAction::MoveUp: return "Move Up";
    case GameAction::MoveDown: return "Move Down";
    case GameAction::MoveLeft: return "Move Left";
    case GameAction::MoveRight: return "Move Right";
    case GameAction::Roll: return "Roll";
    case GameAction::Skill1: return "Skill Slot 1";
    case GameAction::Skill2: return "Skill Slot 2";
    case GameAction::Skill3: return "Skill Slot 3";
    case GameAction::Skill4: return "Skill Slot 4";
    case GameAction::Skill5: return "Skill Slot 5";
    case GameAction::ToggleCharacterSheet: return "Character Sheet";
    case GameAction::ToggleInventory: return "Inventory";
    case GameAction::TogglePassiveTree: return "Passive Tree";
    case GameAction::ToggleSkillGems: return "Skill Gems";
    case GameAction::VendorToggle: return "Talk to Vendor";
    case GameAction::VendorReroll: return "Vendor Reroll Stock";
    default: return "Unknown";
    }
}

std::string KeyBindings::ActionKey(GameAction action) {
    switch (action) {
    case GameAction::MoveUp: return "moveUp";
    case GameAction::MoveDown: return "moveDown";
    case GameAction::MoveLeft: return "moveLeft";
    case GameAction::MoveRight: return "moveRight";
    case GameAction::Roll: return "roll";
    case GameAction::Skill1: return "skill1";
    case GameAction::Skill2: return "skill2";
    case GameAction::Skill3: return "skill3";
    case GameAction::Skill4: return "skill4";
    case GameAction::Skill5: return "skill5";
    case GameAction::ToggleCharacterSheet: return "toggleCharacterSheet";
    case GameAction::ToggleInventory: return "toggleInventory";
    case GameAction::TogglePassiveTree: return "togglePassiveTree";
    case GameAction::ToggleSkillGems: return "toggleSkillGems";
    case GameAction::VendorToggle: return "vendorToggle";
    case GameAction::VendorReroll: return "vendorReroll";
    default: return "unknown";
    }
}

void KeyBindings::SaveToDisk(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return;

    for (size_t i = 0; i < m_bindings.size(); ++i) {
        auto action = static_cast<GameAction>(i);
        out << ActionKey(action) << "=" << KeyToString(m_bindings[i]) << "\n";
    }
}

bool KeyBindings::LoadFromDisk(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;

    std::unordered_map<std::string, std::string> kv;
    std::string line;
    while (std::getline(in, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        kv[line.substr(0, eq)] = line.substr(eq + 1);
    }

    for (size_t i = 0; i < m_bindings.size(); ++i) {
        auto action = static_cast<GameAction>(i);
        auto it = kv.find(ActionKey(action));
        if (it == kv.end()) continue; // not present (older file) -> keep the default

        sf::Keyboard::Key key = StringToKey(it->second);
        if (key != sf::Keyboard::Key::Unknown) {
            m_bindings[i] = key;
        }
    }
    return true;
}
