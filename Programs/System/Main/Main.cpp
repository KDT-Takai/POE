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
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <exception>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

#ifdef _DEBUG
#define ENTRY_POINT int main()
#else
#define ENTRY_POINT int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#endif // _DEBUG

// C++ try/catch (in ENTRY_POINT below) only sees thrown C++ exceptions -- this project
// builds with /EHsc, so a hardware fault (access violation, stack overflow, etc.) is a
// Windows SEH exception that skips it entirely and falls straight to the CRT's generic
// "stopped working" dialog with zero diagnostics. This filter runs first for those:
// logs the exception code/faulting address and writes a minidump (openable in Visual
// Studio/WinDbg to see the exact crashing line) before the same dialog still appears.
LONG WINAPI LogUnhandledSEH(EXCEPTION_POINTERS* info) {
	spdlog::critical("Unhandled SEH exception 0x{:X} at address {}",
		static_cast<unsigned long>(info->ExceptionRecord->ExceptionCode),
		static_cast<void*>(info->ExceptionRecord->ExceptionAddress));
	spdlog::default_logger()->flush();

	HANDLE file = CreateFileA("crash.dmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file != INVALID_HANDLE_VALUE) {
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo{};
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ExceptionPointers = info;
		dumpInfo.ClientPointers = FALSE;
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
			MiniDumpNormal, &dumpInfo, nullptr, nullptr);
		CloseHandle(file);
		spdlog::critical("Wrote crash.dmp");
		spdlog::default_logger()->flush();
	}

	return EXCEPTION_CONTINUE_SEARCH; // let the OS/CRT still show its usual dialog
}

ENTRY_POINT {
	// No log file or crash diagnostics existed before this: spdlog's default logger is
	// console-only, and the console window (and all its scrollback) closes the instant
	// the process dies, so a crash left nothing to inspect afterward. A file sink
	// (flushed on every warning+ so a hard crash doesn't lose the last lines) plus the
	// try/catch below -- logging an uncaught exception's message (e.g.
	// EntityObject::GetComponent<T>()'s "Component not found") before letting it
	// propagate -- turns "it crashed, no idea why" into something inspectable in
	// game.log after the fact.
	try {
		auto fileLogger = spdlog::basic_logger_mt("file_logger", "game.log", true);
		spdlog::set_default_logger(fileLogger);
		spdlog::flush_on(spdlog::level::warn);
		spdlog::set_level(spdlog::level::info);
	}
	catch (const spdlog::spdlog_ex&) {
		// Fall back to the default console-only logger if the log file can't be opened
		// (e.g. read-only install dir); losing diagnostics shouldn't stop the game.
	}

	SetUnhandledExceptionFilter(LogUnhandledSEH);

	try {
		std::unique_ptr<Application> app = std::make_unique<Application>();
		app->run();
	}
	catch (const std::exception& e) {
		spdlog::critical("Unhandled exception, application terminating: {}", e.what());
		spdlog::default_logger()->flush();
		throw;
	}
	catch (...) {
		spdlog::critical("Unhandled non-std exception, application terminating.");
		spdlog::default_logger()->flush();
		throw;
	}
}