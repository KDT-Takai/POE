#pragma once
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <cstdint>
#include "../Singleton/Singleton.h"

class Time : public Singleton<Time> {
    friend class Singleton<Time>;
public:
    Time();

    void Update();

    double GetDeltaTime() const;
    double GetUnscaledDeltaTime() const;
    double GetTotalTime() const;

    void SetTimeScale(double scale);
    double GetTimeScale() const;

    double GetFPS() const;
    void SetTargetFPS(unsigned int fps);
    double GetTargetFPS() const;

    double GetFixedDeltaTime() const;
    double GetAccumulator() const;
    void ConsumeAccumulator(double fixedStep);
    double GetSmoothedAlpha() const;

    void RenderImGui();

private:
    sf::Clock clock;
    sf::Time deltaTime;
    sf::Time totalTime;

    double timeScale;
    double accumulator;
    double currentFps;

    unsigned int targetFPS;

    sf::Time fpsTimer;
    int frameCount;

    const double FIXED_TIME_STEP = 1.0 / 60.0;
};
/* memo
DeltaTime
固定のDeltaTime
描画の保管地　アルファ
フレームレート計測
window->setFramerateLimit(60); // フレームレート制限でできる
ただ、今何フレームやフレームの設定はTimeクラスで管理

Timeクラスに必要なもの
時間の経過管理
タイマー機能
フレームレート制御
時間の加算・比較
時間のスケール
ゲーム内時間と実時間の管理
イベントベースの時間管理


sf::Clockについて
経過時間を測定するためのクラス
主な機能：
	経過時間の取得: sf::Clock は経過時間を計測し、時間が経過するごとにその値を取得できる
	時間のリセット: クロックをリセットすることで、再度時間をゼロから測り始めることができる
主なメソッド：
	getElapsedTime(): 経過時間を sf::Time オブジェクトとして取得
	restart(): クロックをリセットし、経過時間をゼロに戻す
sf::Timeについて
時間の表現と操作を行うクラス
主な機能：
	時間の格納: 時間の長さ（例えば1秒、100ミリ秒など）を格納
	時間の演算: sf::Time オブジェクト同士を加算、減算したり、比較したりできる
	時間の単位変換: 秒、ミリ秒、マイクロ秒など、時間を異なる単位で取得できる
主なメソッド：
	asSeconds(): 時間を秒単位で取得
	asMilliseconds(): 時間をミリ秒単位で取得
	asMicroseconds(): 時間をマイクロ秒単位で取得
	operator+: sf::Time オブジェクト同士の加算をサポート
	operator-: sf::Time オブジェクト同士の減算をサポート
*/