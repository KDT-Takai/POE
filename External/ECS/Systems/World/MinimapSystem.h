#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <algorithm>
#include "../../Registry/Registry.h"
#include "../../Components/World/Map.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "System/Input/InputManager.h"
#include "System/Resource/ResourceManager/ResourceManager.h"

// PoE2風のミニマップ+全体マップ。プレイヤーが実際に歩いた範囲だけをMapComponent::visited
// (円形リビール、kRevealRadiusTiles)に記録し、常時表示の小さいミニマップ(画面右上)と、
// Tabキーで開く全画面の半透明オーバーレイ(矢印キーでパン可能)の両方をこの記録済みタイル
// だけから描画する(未探索は真っ暗のまま)。Tabの開閉自体はGameScene::Updateが他の全画面
// メニュー(PassiveTree/Atlas等)と排他になるよう呼び出す(ToggleFullMap/CloseFullMap)。
class MinimapSystem {
public:
    static constexpr int kRevealRadiusTiles = 7;

    bool IsFullMapOpen() const { return m_fullMapOpen; }
    void ToggleFullMap() {
        m_fullMapOpen = !m_fullMapOpen;
        if (m_fullMapOpen) m_panOffsetTiles = { 0.0f, 0.0f };
    }
    void CloseFullMap() { m_fullMapOpen = false; }

    // 探索範囲の記録(常時)と、全体マップが開いている間の矢印キーによるパン。
    // isPaused判定の外側から毎フレーム呼ぶ想定(GameScene::Update参照)。
    void RevealAndPan(Registry& registry, Entity playerEntity, float dt) {
        MapComponent* map = FindMap(registry);
        if (!map || !registry.IsValid(playerEntity) || !registry.HasComponent<TransformComponent>(playerEntity)) return;

        auto& trans = registry.GetComponent<TransformComponent>(playerEntity);
        sf::Vector2i playerTile = map->WorldToTile(trans.position.x, trans.position.y);
        map->RevealCircle(playerTile.x, playerTile.y, kRevealRadiusTiles);

        if (!m_fullMapOpen) return;

        auto& keyInput = InputManager::Instance().GetKeyInput();
        sf::Vector2f pan(0.0f, 0.0f);
        if (keyInput.GetKey(sf::Keyboard::Key::Left)) pan.x -= 1.0f;
        if (keyInput.GetKey(sf::Keyboard::Key::Right)) pan.x += 1.0f;
        if (keyInput.GetKey(sf::Keyboard::Key::Up)) pan.y -= 1.0f;
        if (keyInput.GetKey(sf::Keyboard::Key::Down)) pan.y += 1.0f;
        if (pan.x == 0.0f && pan.y == 0.0f) return;

        float len = std::sqrt(pan.x * pan.x + pan.y * pan.y);
        pan /= len;
        m_panOffsetTiles += pan * kPanTilesPerSecond * dt;
        float maxPan = static_cast<float>((std::max)(map->width, map->height));
        m_panOffsetTiles.x = std::clamp(m_panOffsetTiles.x, -maxPan, maxPan);
        m_panOffsetTiles.y = std::clamp(m_panOffsetTiles.y, -maxPan, maxPan);
    }

    // 画面右上、常時表示の小さいミニマップ。target.getView()を変更するので、呼び出し側の
    // ビューを事前にデフォルトへ戻しておくこと(GameScene::Render参照)。
    void RenderMinimap(Registry& registry, sf::RenderTarget& target, Entity playerEntity) {
        MapComponent* map = FindMap(registry);
        if (!map || map->width <= 0 || !registry.IsValid(playerEntity) || !registry.HasComponent<TransformComponent>(playerEntity)) return;

        auto& trans = registry.GetComponent<TransformComponent>(playerEntity);
        sf::Vector2f playerTileF(trans.position.x / map->tileSize, trans.position.y / map->tileSize);

        sf::Vector2u winSize = target.getSize();
        float boxSize = 220.0f;
        float margin = 16.0f;
        sf::Vector2f boxCenter(static_cast<float>(winSize.x) - margin - boxSize / 2.0f, margin + boxSize / 2.0f);

        DrawFrame(target, boxCenter, boxSize, boxSize);

        sf::FloatRect clip({ boxCenter.x - boxSize / 2.0f, boxCenter.y - boxSize / 2.0f }, { boxSize, boxSize });
        target.setView(MakeClipView(clip, winSize));
        // MakeClipViewの reset rect / viewport はどちらも絶対スクリーン座標系(x,y)に
        // 揃えてあるため(PassiveTreeSystemのtreeViewと同じ手法、1ビュー単位=1px)、
        // ここで渡す中心座標もローカル(boxSize/2)ではなく絶対座標のboxCenterを使う。
        DrawTiles(*map, target, playerTileF, boxCenter, kMinimapPixelsPerTile, boxSize, boxSize);
        DrawPlayerMarker(target, boxCenter);
        target.setView(target.getDefaultView());
    }

    // Tabキーで開く全画面オーバーレイ。isFullMapOpenでない間は何もしない。
    void RenderFullMap(Registry& registry, sf::RenderTarget& target, Entity playerEntity) {
        if (!m_fullMapOpen) return;
        MapComponent* map = FindMap(registry);
        if (!map || map->width <= 0 || !registry.IsValid(playerEntity) || !registry.HasComponent<TransformComponent>(playerEntity)) return;

        auto& trans = registry.GetComponent<TransformComponent>(playerEntity);
        sf::Vector2f playerTileF(trans.position.x / map->tileSize, trans.position.y / map->tileSize);
        sf::Vector2f focusTileF = playerTileF + m_panOffsetTiles;

        sf::Vector2u winSize = target.getSize();
        float w = static_cast<float>(winSize.x);
        float h = static_cast<float>(winSize.y);

        sf::RectangleShape dim({ w, h });
        dim.setFillColor(sf::Color(0, 0, 0, 165));
        target.draw(dim);

        sf::Vector2f center(w / 2.0f, h / 2.0f);
        float viewW = (std::max)(200.0f, w - 120.0f);
        float viewH = (std::max)(200.0f, h - 160.0f);

        sf::FloatRect clip({ center.x - viewW / 2.0f, center.y - viewH / 2.0f }, { viewW, viewH });
        DrawFrame(target, center, viewW, viewH);

        target.setView(MakeClipView(clip, winSize));
        DrawTiles(*map, target, focusTileF, center, kFullMapPixelsPerTile, viewW, viewH);

        // プレイヤー自身の画面位置はfocusTileFからのオフセットで求める(パン中は中心とズレる)。
        sf::Vector2f playerScreenPos = center + (playerTileF - focusTileF) * kFullMapPixelsPerTile;
        DrawPlayerMarker(target, playerScreenPos);
        target.setView(target.getDefaultView());

        auto font = ResourceManager::Instance().getFont("Assets/Fonts/NotoSansJP-Regular.ttf");
        if (font) {
            std::string hintStr = "TAB: 地図を閉じる　矢印キー: 移動";
            sf::Text hint(*font, sf::String::fromUtf8(hintStr.begin(), hintStr.end()), 16);
            hint.setFillColor(sf::Color::White);
            hint.setOutlineColor(sf::Color::Black);
            hint.setOutlineThickness(2.0f);
            sf::FloatRect bounds = hint.getLocalBounds();
            hint.setOrigin({ bounds.position.x + bounds.size.x / 2.0f, 0.0f });
            hint.setPosition({ w / 2.0f, h - 40.0f });
            target.draw(hint);
        }
    }

private:
    static constexpr float kPanTilesPerSecond = 16.0f;
    static constexpr float kMinimapPixelsPerTile = 6.0f;
    static constexpr float kFullMapPixelsPerTile = 8.0f;

    bool m_fullMapOpen = false;
    sf::Vector2f m_panOffsetTiles{ 0.0f, 0.0f };

    static MapComponent* FindMap(Registry& registry) {
        auto view = registry.View<MapComponent>();
        if (view.empty()) return nullptr;
        return &registry.GetComponent<MapComponent>(view[0]);
    }

    // PassiveTreeSystem/AtlasSystem等と同じ「ビューポートクリッピング」手法(AI/DECISIONS.md
    // 参照): 1ビュー単位=1pxのままclipRectの範囲だけに描画を絞る。以後clipRectを基準にした
    // 絶対スクリーン座標をそのまま描画すればよい。
    static sf::View MakeClipView(sf::FloatRect clipRect, sf::Vector2u winSize) {
        sf::View view(clipRect);
        view.setViewport(sf::FloatRect(
            { clipRect.position.x / static_cast<float>(winSize.x), clipRect.position.y / static_cast<float>(winSize.y) },
            { clipRect.size.x / static_cast<float>(winSize.x), clipRect.size.y / static_cast<float>(winSize.y) }));
        return view;
    }

    static void DrawFrame(sf::RenderTarget& target, sf::Vector2f center, float w, float h) {
        sf::RectangleShape bg({ w, h });
        bg.setOrigin({ w / 2.0f, h / 2.0f });
        bg.setPosition(center);
        bg.setFillColor(sf::Color(10, 10, 15, 170));
        bg.setOutlineColor(sf::Color(150, 150, 160, 220));
        bg.setOutlineThickness(2.0f);
        target.draw(bg);
    }

    static sf::Color TileColor(TileType type) {
        switch (type) {
        case TileType::Dirt: return sf::Color(150, 130, 95);
        case TileType::Wood: return sf::Color(80, 220, 110);  // スタート地点
        case TileType::Grass: return sf::Color(255, 210, 60); // ゴール/ボス部屋方向
        case TileType::Stone: return sf::Color(35, 35, 40);
        case TileType::Bedrock: return sf::Color(10, 10, 12);
        default: return sf::Color(0, 0, 0, 0);
        }
    }

    // centerTileF(タイル座標、小数可)がscreenCenter(px)に来るように、探索済みタイルだけを
    // 1本のVertexArrayへまとめて1回のdraw callで描く(MapRenderSystemと同じくタイル毎の
    // 個別draw callは処理落ちの原因になるため)。
    static void DrawTiles(const MapComponent& map, sf::RenderTarget& target, sf::Vector2f centerTileF,
        sf::Vector2f screenCenter, float pixelsPerTile, float rectW, float rectH) {
        float halfTilesX = (rectW / 2.0f) / pixelsPerTile + 1.0f;
        float halfTilesY = (rectH / 2.0f) / pixelsPerTile + 1.0f;
        int minX = (std::max)(0, static_cast<int>(std::floor(centerTileF.x - halfTilesX)));
        int maxX = (std::min)(map.width - 1, static_cast<int>(std::ceil(centerTileF.x + halfTilesX)));
        int minY = (std::max)(0, static_cast<int>(std::floor(centerTileF.y - halfTilesY)));
        int maxY = (std::min)(map.height - 1, static_cast<int>(std::ceil(centerTileF.y + halfTilesY)));
        if (minX > maxX || minY > maxY) return;

        sf::VertexArray quads(sf::PrimitiveType::Triangles);
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (!map.IsVisited(x, y)) continue;
                sf::Color color = TileColor(map.GetTile(x, y));
                if (color.a == 0) continue;

                sf::Vector2f p((static_cast<float>(x) - centerTileF.x) * pixelsPerTile + screenCenter.x,
                                (static_cast<float>(y) - centerTileF.y) * pixelsPerTile + screenCenter.y);
                float s = pixelsPerTile + 0.6f; // わずかに重ねて継ぎ目の隙間を消す
                sf::Vector2f p0 = p;
                sf::Vector2f p1 = p + sf::Vector2f(s, 0.0f);
                sf::Vector2f p2 = p + sf::Vector2f(s, s);
                sf::Vector2f p3 = p + sf::Vector2f(0.0f, s);
                quads.append(sf::Vertex{ p0, color });
                quads.append(sf::Vertex{ p1, color });
                quads.append(sf::Vertex{ p2, color });
                quads.append(sf::Vertex{ p0, color });
                quads.append(sf::Vertex{ p2, color });
                quads.append(sf::Vertex{ p3, color });
            }
        }
        if (quads.getVertexCount() > 0) target.draw(quads);
    }

    static void DrawPlayerMarker(sf::RenderTarget& target, sf::Vector2f screenPos) {
        sf::CircleShape dot(5.0f);
        dot.setOrigin({ 5.0f, 5.0f });
        dot.setPosition(screenPos);
        dot.setFillColor(sf::Color(80, 220, 255));
        dot.setOutlineColor(sf::Color::White);
        dot.setOutlineThickness(1.5f);
        target.draw(dot);
    }
};
