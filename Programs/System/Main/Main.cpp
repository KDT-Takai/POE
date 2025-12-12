// SFML 3.0 Documentation site
// https://www.sfml-dev.org/documentation/3.0.2/index.html
// --------------------------------
// SFML - Simple and Fast Multimedia Library
// --------------------------------
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
// --------------------------------
// Headers
// --------------------------------
#include "Application.h"

#ifdef _DEBUG
#define ENTRY_POINT int main()
#else
#include<Windows.h>
#define ENTRY_POINT int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#endif // _DEBUG

ENTRY_POINT {
	std::unique_ptr<Application> app = std::make_unique<Application>();
	app->run();
}