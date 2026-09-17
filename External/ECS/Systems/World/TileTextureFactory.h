#pragma once
#include <SFML/Graphics.hpp>
#include <random>
#include <array>
#include <cstdint>
#include <algorithm>
#include <spdlog/spdlog.h>
#include "../../Components/World/Map.h"

// No hand-authored tile art exists yet ("テクスチャを追加してほしい"), so this
// procedurally paints a small noisy texture per TileType once at startup (cached, see
// Get) instead of MapRenderSystem's old flat single-color fill. Each texture is bigger
// than one tile (kAtlasSize) so MapRenderSystem can sample a different tileSize-sized
// window per grid cell (see TextureRectFor) -- that's what keeps a floor of same-type
// tiles from reading as an obviously repeating stamp.
class TileTextureFactory {
public:
    static constexpr unsigned int kAtlasSize = 256;

    static sf::Texture& Get(TileType type) {
        static std::array<sf::Texture, 6> textures = BuildAll();
        return textures[static_cast<size_t>(type)];
    }

    // Deterministic per-cell offset into the kAtlasSize atlas so the same map always
    // looks the same across frames/reloads, but neighboring tiles don't visibly repeat
    // the exact same pixels.
    static sf::IntRect TextureRectFor(int tileCol, int tileRow, int tileSizeInt) {
        unsigned int span = tileSizeInt > 0 ? static_cast<unsigned int>(tileSizeInt) : 1;
        unsigned int maxOffset = span < kAtlasSize ? kAtlasSize - span : 0;
        unsigned int ox = maxOffset > 0 ? static_cast<unsigned int>(tileCol * 37 + tileRow * 91) % (maxOffset + 1) : 0;
        unsigned int oy = maxOffset > 0 ? static_cast<unsigned int>(tileCol * 71 + tileRow * 53) % (maxOffset + 1) : 0;
        return sf::IntRect({ static_cast<int>(ox), static_cast<int>(oy) }, { tileSizeInt, tileSizeInt });
    }

private:
    static std::array<sf::Texture, 6> BuildAll() {
        std::array<sf::Texture, 6> textures;
        textures[static_cast<size_t>(TileType::Air)] = MakeSolid(sf::Color::Transparent);
        textures[static_cast<size_t>(TileType::Dirt)] = MakeFloor(sf::Color(96, 82, 64), sf::Color(70, 58, 44), 0xD127u);
        textures[static_cast<size_t>(TileType::Stone)] = MakeBrick(sf::Color(58, 58, 66), sf::Color(28, 28, 32));
        textures[static_cast<size_t>(TileType::Bedrock)] = MakeFloor(sf::Color(18, 18, 21), sf::Color(8, 8, 10), 0xBEEFu);
        textures[static_cast<size_t>(TileType::Wood)] = MakePlanks(sf::Color(120, 84, 46), sf::Color(90, 60, 30));
        textures[static_cast<size_t>(TileType::Grass)] = MakeFloor(sf::Color(58, 118, 54), sf::Color(40, 92, 38), 0x9A5Eu);
        return textures;
    }

    static sf::Texture FinishTexture(sf::Image& image, const char* label) {
        sf::Texture tex;
        if (!tex.loadFromImage(image)) {
            spdlog::error("TileTextureFactory: failed to build '{}' texture", label);
        }
        tex.setRepeated(true);
        return tex;
    }

    static sf::Texture MakeSolid(sf::Color color) {
        sf::Image image({ kAtlasSize, kAtlasSize }, color);
        return FinishTexture(image, "solid");
    }

    // Speckled floor: base color with per-pixel random-brightness noise plus scattered
    // darker flecks, matching a rough stone/dirt/grass floor look without a sprite sheet.
    static sf::Texture MakeFloor(sf::Color base, sf::Color fleck, unsigned int seed) {
        sf::Image image({ kAtlasSize, kAtlasSize }, base);
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> speckleRoll(0.0f, 1.0f);
        std::uniform_int_distribution<int> jitter(-14, 14);
        for (unsigned int y = 0; y < kAtlasSize; ++y) {
            for (unsigned int x = 0; x < kAtlasSize; ++x) {
                if (speckleRoll(rng) < 0.10f) {
                    image.setPixel({ x, y }, fleck);
                } else {
                    image.setPixel({ x, y }, Shade(base, jitter(rng)));
                }
            }
        }
        return FinishTexture(image, "floor");
    }

    // Brick wall: dark mortar grid lines over a lighter brick fill, offset every other
    // row like real brickwork.
    static sf::Texture MakeBrick(sf::Color base, sf::Color mortar) {
        sf::Image image({ kAtlasSize, kAtlasSize }, base);
        constexpr unsigned int brickW = 32, brickH = 16;
        std::mt19937 rng(54321);
        std::uniform_int_distribution<int> jitter(-10, 10);
        for (unsigned int y = 0; y < kAtlasSize; ++y) {
            unsigned int rowIndex = y / brickH;
            unsigned int xOffset = (rowIndex % 2 == 0) ? 0 : brickW / 2;
            for (unsigned int x = 0; x < kAtlasSize; ++x) {
                bool mortarLine = (y % brickH == 0) || (((x + xOffset) % brickW) == 0);
                image.setPixel({ x, y }, mortarLine ? mortar : Shade(base, jitter(rng)));
            }
        }
        return FinishTexture(image, "brick");
    }

    // Wood planks: vertical seams every plankW pixels over grainy noise.
    static sf::Texture MakePlanks(sf::Color base, sf::Color seam) {
        sf::Image image({ kAtlasSize, kAtlasSize }, base);
        constexpr unsigned int plankW = 24;
        std::mt19937 rng(2468);
        std::uniform_int_distribution<int> jitter(-10, 10);
        for (unsigned int y = 0; y < kAtlasSize; ++y) {
            for (unsigned int x = 0; x < kAtlasSize; ++x) {
                bool seamLine = (x % plankW) == 0;
                image.setPixel({ x, y }, seamLine ? seam : Shade(base, jitter(rng)));
            }
        }
        return FinishTexture(image, "planks");
    }

    static sf::Color Shade(sf::Color c, int delta) {
        auto clamp8 = [](int v) { return static_cast<std::uint8_t>(std::clamp(v, 0, 255)); };
        return sf::Color(clamp8(c.r + delta), clamp8(c.g + delta), clamp8(c.b + delta), c.a);
    }
};
