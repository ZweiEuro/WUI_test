
/*
 * This program uses the Allegro game library to display a blank window.
 *
 * It initializes the display and starts up the main game loop. The
 * game loop only checks for two events: timer (determined by the FPS)
 * and display close (when the user tries to close the window).
 *
 * http://www.damienradtke.org/building-a-mario-clone-with-allegro
 */

#include <stdio.h>
#include <allegro5/allegro.h>

#include <include/cef_client.h>

#include "BrowserClient.hpp"

const float FPS = 60;

int main(int argc, char *argv[])
{
	CefMainArgs args(argc, argv);

	{
		int result = CefExecuteProcess(args, nullptr, nullptr);
		// checkout CefApp, derive it and set it as second parameter, for more control on
		// command args and resources.
		if (result >= 0) // child proccess has endend, so exit.
		{
			return result;
		}
		if (result == -1)
		{
			// we are here in the father proccess.
		}
	}

	{
		CefSettings settings;

		// checkout detailed settings options http://magpcss.org/ceforum/apidocs/projects/%28default%29/_cef_settings_t.html
		// nearly all the settings can be set via args too.
		// settings.multi_threaded_message_loop = true; // not supported, except windows
		// CefString(&settings.browser_subprocess_path).FromASCII("sub_proccess path, by default uses and starts this executeable as child");
		// CefString(&settings.cache_path).FromASCII("");
		// CefString(&settings.log_file).FromASCII("");
		// settings.log_severity = LOGSEVERITY_DEFAULT;
		// CefString(&settings.resources_dir_path).FromASCII("");
		// CefString(&settings.locales_dir_path).FromASCII("");

		// settings.windowless_rendering_enabled = true;

		bool result = CefInitialize(args, settings, nullptr, nullptr);
		// CefInitialize creates a sub-proccess and executes the same executeable, as calling CefInitialize, if not set different in settings.browser_subprocess_path
		// if you create an extra program just for the childproccess you only have to call CefExecuteProcess(...) in it.
		if (!result)
		{
			// handle error
			return -1;
		}
	}

	// init renderer and display

	RenderHandler *renderHandler = new RenderHandler();

	// create browser-window
	CefRefPtr<CefBrowser> browser;
	CefRefPtr<BrowserClient> browserClient;
	{
		CefWindowInfo window_info;
		CefBrowserSettings browserSettings;

		browserSettings.windowless_frame_rate = 60; // 30 is default

		// in linux set a gtk widget, in windows a hwnd. If not available set nullptr - may cause some render errors, in context-menu and plugins.
		// std::size_t windowHandle = 0;
		// renderSystem->getAutoCreatedWindow()->getCustomAttribute("WINDOW", &windowHandle);
		// window_info.SetAsWindowless(0); // false means no transparency (site background colour)

		browserClient = new BrowserClient(renderHandler);

		browser = CefBrowserHost::CreateBrowserSync(window_info, browserClient, "http://google.com", browserSettings, nullptr, nullptr);

		// inject user-input by calling - non-trivial for non-windows - checkout the cefclient source and the platform specific cpp, like cefclient_osr_widget_gtk.cpp for linux
		// browser->GetHost()->SendKeyEvent(...);
		// browser->GetHost()->SendMouseMoveEvent(...);
		// browser->GetHost()->SendMouseClickEvent(...);
		// browser->GetHost()->SendMouseWheelEvent(...);
	}

	renderHandler->renderLoop();

	{
		browser = nullptr;
		browserClient = nullptr;
		CefShutdown();

		delete renderHandler;
	}

	return 0;
}