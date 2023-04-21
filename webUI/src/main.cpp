
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
#include <allegro5/allegro_x.h>

#ifdef ALLEGRO_WINDOWS
#include <allegro5/allegro_windows.h>
#endif

#ifdef ALLEGRO_UNIX
#include <allegro5/allegro_x.h>
#endif

#include <include/cef_client.h>

#include "BrowserClient.hpp"
#include "Objects/Ball.hpp"
#include "Input/InputManager.hpp"

const float FPS = 60;

CefRefPtr<WUI::RenderHandler> renderHandler;
CefRefPtr<CefBrowser> browser;
CefRefPtr<WUI::BrowserClient> browserClient;

int main(int argc, char *argv[])
{
	CefMainArgs args(argc, argv);

	{

		int result = CefExecuteProcess(args, nullptr, nullptr);
		// checkout CefApp, derive it and set it as second parameter, for more control on
		// command args and resources.
		if (result >= 0) // child proccess has endend, so exit.
		{
			exit(result);
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

		settings.log_severity = LOGSEVERITY_INFO;
		settings.windowless_rendering_enabled = true;

#if !defined(CEF_USE_SANDBOX)
		settings.no_sandbox = true;
#endif

		// init custom scheme for local files

		bool result = CefInitialize(args, settings, nullptr, nullptr);

		// CefInitialize creates a sub-proccess and executes the same executeable, as calling CefInitialize, if not set different in settings.browser_subprocess_path
		// if you create an extra program just for the childproccess you only have to call CefExecuteProcess(...) in it.
		if (!result)
		{
			// handle error
			exit(-2);
		}
	}

	// init renderer and display

	renderHandler = new WUI::RenderHandler();
	std::string current_dir = "";
	{
		auto charp = al_get_current_directory();
		current_dir = std::string(charp) + "/";
		free(charp);
	}

	// create browser-window

	{
		CefWindowInfo window_info;
		window_info.SetAsWindowless(0); // false means no transparency (site background colour)

		browserClient = new WUI::BrowserClient(renderHandler);

		CefBrowserSettings browserSettings;
		browserSettings.windowless_frame_rate = 60; // 30 is default

		auto path = "file://" + current_dir + "html/index.html";

		browser = CefBrowserHost::CreateBrowserSync(window_info, browserClient.get(), path, browserSettings, nullptr, nullptr);

		WUI::InputManager::instance(browser->GetHost());

		std::thread([=]() -> void
					{
                  WUI::InputManager::instance()->wait_for_key(ALLEGRO_KEY_ESCAPE);
				  	DLOG(INFO) << "Shutting down";
				                    renderHandler->shutdown();
				  WUI::InputManager::instance()->shutdown();

                  exit(0); })
			.detach();
	}

	renderHandler->renderLoop();

	{
		browser = nullptr;
		browserClient = nullptr;
		CefShutdown();
	}

	return 0;
}