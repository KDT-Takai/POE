#pragma once
#include "../ECS.h"
#include "../Components/Components.h"
#include "imgui.h"

class EditorSystem {
    Entity m_selectedEntity = -1;

public:
    void SetSelectedEntity(Entity entity) {

        m_selectedEntity = entity;

    }

    void RenderImGui(Registry& registry) {
        ImGui::Begin("Object Editor");
        ImGui::Separator();

        // ヒエラルキー
        ImGui::Text("Hierarchy");

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
        ImGui::End();

        // インスペクター
        ImGui::Begin("Inspector");
        if (m_selectedEntity != -1) {
            DrawComponents(registry, m_selectedEntity);
        }
        else {
            ImGui::Text("No entity selected.");
        }
        ImGui::End();
    }

private:
    void DrawComponents(Registry& registry, Entity entity) {
        // タグ
        if (registry.HasComponent<TagComponent>(entity))
        {
            auto& tag = registry.GetComponent<TagComponent>(entity);
            char buffer[256];
            strncpy_s(buffer, tag.name.c_str(), sizeof(buffer));
            if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
                tag.name = buffer;
            }
        }
        // 削除ボタン
        if (registry.HasComponent<TagComponent>(entity))
        {
            auto& tag = registry.GetComponent<TagComponent>(entity);
            ImGui::Text("Tag: %s", tag.name.c_str());
            if (ImGui::Button("Delete"))
            {
                registry.RemoveComponent<TagComponent>(entity);
                registry.DestroyEntity(entity);
            }
        }
        ImGui::Separator();
        // Transform
        if (registry.HasComponent<TransformComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& t = registry.GetComponent<TransformComponent>(entity);
                ImGui::DragFloat2("Position", &t.position.x);
                ImGui::DragFloat("Rotation", &t.rotation);
                ImGui::DragFloat2("Scale", &t.scale.x, 0.01f);
            }
        }
        // Circle (円の設定)
        if (registry.HasComponent<CircleComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Circle Shape", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& c = registry.GetComponent<CircleComponent>(entity);
                ImGui::DragFloat("Radius", &c.radius);

                float color[4] = { c.color.r / 255.f, c.color.g / 255.f, c.color.b / 255.f, c.color.a / 255.f };
                if (ImGui::ColorEdit4("Color", color))
                {
                    c.color = sf::Color(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255);
                }
                ImGui::Checkbox("Visible", &c.isVisible);
            }
        }
        // Sprite (スプライトの設定)
        if (registry.HasComponent<SpriteComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // (既存のSprite編集コード...)
            }
        }
    }
};