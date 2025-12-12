#include "ResourceManager.h"
#include <spdlog/spdlog.h> // ログ出力用

template <typename T>
void ResourceManager::loadResourcesFromDirectory( ResourceCache<T>& cache, const std::string& folderPath, const std::vector<std::string>& extensions) {
    // フォルダが存在するか確認
    if (!std::filesystem::exists(folderPath)) {
        spdlog::warn("ResourceManager: Directory not found: {}", folderPath);
        return;
    }

    spdlog::info("ResourceManager: Scanning directory '{}'...", folderPath);

    // 再帰的イテレータでサブフォルダも含めて走査
    for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            // ファイルパスと拡張子を取得
//            std::string filePath = entry.path().string();
            std::string filePath = entry.path().generic_string();
            std::string ext = entry.path().extension().string();

            // 拡張子を小文字に変換して比較しやすくする (.PNG -> .png)
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            // 指定された拡張子リストに含まれているかチェック
            for (const auto& targetExt : extensions) {
                if (ext == targetExt) {
                    // ResourceCacheのloadを呼び出す
                    // Windowsのパス区切り文字(\)を(/)に統一したい場合はここで置換処理を入れても良い
                    cache.load(filePath);
                    break;
                }
            }
        }
    }
}

// テクスチャ一括読み込みの実装
void ResourceManager::loadAllTextures(const std::string& folderPath) {
    // 読み込み対象の拡張子リスト
    std::vector<std::string> extensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };
    loadResourcesFromDirectory(textures, folderPath, extensions);
}

// フォント一括読み込みの実装
void ResourceManager::loadAllFonts(const std::string& folderPath) {
    std::vector<std::string> extensions = { ".ttf", ".otf" };
    loadResourcesFromDirectory(fonts, folderPath, extensions);
}

// 効果音一括読み込みの実装
void ResourceManager::loadAllSounds(const std::string& folderPath) {
    std::vector<std::string> extensions = { ".wav", ".ogg", ".mp3" }; // sf::SoundBufferはmp3も読める場合があるがwav/ogg推奨
    loadResourcesFromDirectory(soundBuffers, folderPath, extensions);
}

// --- シェーダーの取得 ---
std::shared_ptr<sf::Shader> ResourceManager::getShader(const std::string& filename, sf::Shader::Type type) {
    // キャッシュ確認
    auto it = shaders.find(filename);
    if (it != shaders.end())
    {
        return it->second;
    }

    // 新規ロード
    auto shader = std::make_shared<sf::Shader>();
    // シェーダーは引数(type)が必要なので loadFromFile を直接呼ぶ
    if (!shader->loadFromFile(filename, type))
    {
        spdlog::error("ResourceManager: Failed to load shader: {}", filename);
        return nullptr;
    }

    // 登録
    shaders[filename] = shader;
    spdlog::info("ResourceManager: Loaded shader: {}", filename);
    return shader;
}

// BGM再生
void ResourceManager::playMusic(const std::string& filename) {
    // すでに再生中のものがあれば止める
    if (music)
    {
        music->stop();
    }

    // 新しいMusicインスタンスを作成
    music = std::make_unique<sf::Music>();

    if (music->openFromFile(filename))
    {
        music->setLooping(true);
        music->play();
        spdlog::info("ResourceManager: Playing music: {}", filename);
    }
    else
    {
        spdlog::error("ResourceManager: Failed to open music: {}", filename);
    }
}

// BGM停止
void ResourceManager::stopMusic() {
    if (music)
    {
        music->stop();
        spdlog::info("ResourceManager: Music stopped.");
    }
}

// BGM一時停止
void ResourceManager::pauseMusic() {
    if (music && music->getStatus() == sf::SoundSource::Status::Playing)
    {
        music->pause();
        spdlog::info("ResourceManager: Music paused.");
    }
}

// BGM再開
void ResourceManager::resumeMusic() {
    if (music && music->getStatus() == sf::SoundSource::Status::Paused)
    {
        music->play();
        spdlog::info("ResourceManager: Music resumed.");
    }
}

/* memo
今後 C++17のfilesystemを導入し自動読み込み化させる
自動化させるなら必須
https://qiita.com/ueken0307/items/4d866bb78e9f4fe6232d
https://cpprefjp.github.io/reference/filesystem.html
*/