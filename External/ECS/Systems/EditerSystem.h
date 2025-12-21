#pragma once
#include "../ECS.h"
#include "../Components/Components.h"
#include "imgui.h"
#include <System/DebugGui/DebugGui.h>

class EditorSystem {
    Entity m_selectedEntity = -1;

public:
    void SetSelectedEntity(Entity entity) {

        m_selectedEntity = entity;

    }

    void RenderImGui(Registry& registry) {
        DebugGui::Begin("Object Editor###ObjectEditor", "オブジェクトエディター###ObjectEditor");
        ImGui::Separator();

        // ヒエラルキー
        DebugGui::Text("Hierarchy", "ヒエラルキー");

        std::vector<Entity> toDestroy;
        for (auto entity : registry.GetEntities()) {
            std::string label = "Entity " + std::to_string(entity);
            if (registry.HasComponent<TagComponent>(entity))
            {
                label = registry.GetComponent<TagComponent>(entity).name + " (" + std::to_string(entity) + ")";
            }

            ImGui::PushID(entity);
            if (ImGui::Selectable(label.c_str(), m_selectedEntity == entity))
            {
                m_selectedEntity = entity;
            }
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete"))
                {
                    toDestroy.push_back(entity);
                    if (m_selectedEntity == entity) m_selectedEntity = -1;
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        for (auto e : toDestroy) registry.DestroyEntity(e);
        DebugGui::End();

        // インスペクター
        DebugGui::Begin("Inspector###Inspector", "インスペクター###Inspector");
        if (m_selectedEntity != -1) {
            DrawComponents(registry, m_selectedEntity);
        }
        else {
            DebugGui::Text("No entity selected.", "エンティティが選択されていません");
        }
        DebugGui::End();
    }

private:
    void DrawComponents(Registry& registry, Entity entity) {
        // タグ
        if (registry.HasComponent<TagComponent>(entity))
        {
            auto& tag = registry.GetComponent<TagComponent>(entity);
            char buffer[256];
            strncpy_s(buffer, tag.name.c_str(), sizeof(buffer));
            if (DebugGui::InputText("Name", "名前", buffer, sizeof(buffer))) {
                tag.name = buffer;
            }
        }
        // 削除ボタン
        if (registry.HasComponent<TagComponent>(entity))
        {
            auto& tag = registry.GetComponent<TagComponent>(entity);
            DebugGui::Text("Tag: %s", "タグ: %s", tag.name.c_str());
            if (DebugGui::Button("Delete", "削除"))
            {
                registry.RemoveComponent<TagComponent>(entity);
                registry.DestroyEntity(entity);
            }
        }
        ImGui::Separator();
        // Transform
        if (registry.HasComponent<TransformComponent>(entity))
        {
            if (DebugGui::CollapsingHeader("Transform", "トランスフォーム", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& t = registry.GetComponent<TransformComponent>(entity);
                DebugGui::DragFloat2("Position", "座標", &t.position.x);
                DebugGui::DragFloat("Rotation", "角度", &t.rotation);
                DebugGui::DragFloat2("Scale", "スケール", &t.scale.x, 0.01f);
            }
        }
        // Circle (円の設定)
        if (registry.HasComponent<CircleComponent>(entity))
        {
            if (DebugGui::CollapsingHeader("Circle Shape", "円形", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& c = registry.GetComponent<CircleComponent>(entity);
                DebugGui::DragFloat("Radius", "半径", &c.radius);

                float color[4] = { c.color.r / 255.f, c.color.g / 255.f, c.color.b / 255.f, c.color.a / 255.f };
                if (DebugGui::ColorEdit4("Color", "カラー", color))
                {
                    c.color = sf::Color(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255);
                }
                DebugGui::Checkbox("Visible", "表示", &c.isVisible);
            }
        }
        // Sprite (スプライトの設定)
        if (registry.HasComponent<SpriteComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // 既存のSprite編集コード
            }
        }
    }
};