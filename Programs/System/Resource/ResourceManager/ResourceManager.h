#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <filesystem>                       // ファイル自動読み込み用
#include <algorithm>
#include "../../Singleton/Singleton.h"
#include "../ResourceCache/ResourceCache.h"

// リソースマネージャー
class ResourceManager : public Singleton<ResourceManager> {
protected:
    friend class Singleton<ResourceManager>;
private:

    ResourceCache<sf::Texture>      textures;     // 画像
    ResourceCache<sf::Font>         fonts;        // フォント
    ResourceCache<sf::SoundBuffer>  soundBuffers; // 効果音 (SE)
	std::unordered_map<std::string, std::shared_ptr<sf::Shader>> shaders; // シェーダーキャッシュ
	std::unique_ptr<sf::Music> music;      // BGM再生用

    template <typename T>
	// 指定フォルダ(サブフォルダ含む)のリソースを全て読み込む
    void loadResourcesFromDirectory( ResourceCache<T>& cache, const std::string& folderPath, const std::vector<std::string>& extensions);
public:
    // 指定フォルダ(サブフォルダ含む)の画像を全て読み込む
    void loadAllTextures(const std::string& folderPath = "Assets/Textures");
    // 指定フォルダのフォントを全て読み込む
    void loadAllFonts(const std::string& folderPath = "Assets/Fonts");
    // 指定フォルダのSEを全て読み込む
    void loadAllSounds(const std::string& folderPath = "Assets/Sounds");

    // 画像
    void loadTexture(const std::string& path) { textures.load(path); }
    std::shared_ptr<sf::Texture> getTexture(const std::string& path) { return textures.get(path); }
    // フォント
    void loadFont(const std::string& path) { fonts.load(path); }
    std::shared_ptr<sf::Font> getFont(const std::string& path) { return fonts.get(path); }
    // 効果音
    void loadSound(const std::string& path) { soundBuffers.load(path); }
    std::shared_ptr<sf::SoundBuffer> getSound(const std::string& path) { return soundBuffers.get(path); }

    // テクスチャ参照用
    ResourceCache<sf::Texture>& texture() { return textures; }
    // フォント参照用
    ResourceCache<sf::Font>& font() { return fonts; }
    // 効果音参照用
    ResourceCache<sf::SoundBuffer>& sound() { return soundBuffers; }
	// シェーダー取得
    std::shared_ptr<sf::Shader> getShader(const std::string& filename, sf::Shader::Type type);

    // BGM操作
    void playMusic(const std::string& filename);
    void stopMusic();
    void pauseMusic();
    void resumeMusic();
    
};
/* memo
リソースマネージャー作成にあたってのメモ
画像、音声、フォントなどのリソースを一元管理するクラス
画像管理クラス、音声管理クラス、フォント管理クラスなどをまとめる
*/