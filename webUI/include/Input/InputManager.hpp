#pragma once
#include <allegro5/allegro.h>
#include <mutex>
#include <thread>
#include "Math/vec.hpp"
#include "include/cef_browser.h"

namespace WUI
{

    class InputManager
    {
    private:
        static InputManager *m_instance;
        static CefRefPtr<CefBrowserHost> m_browser_host;

        ALLEGRO_EVENT_QUEUE *m_InputManager_event_queue; // main queue for the GameManager

        ALLEGRO_EVENT_SOURCE m_InputManager_event_source; // where the manager tells "wait for keys" if something was pressed or not
        vec2i m_mouse_state;
        std::mutex l_mouse_state;

        std::thread m_input_thread;

        InputManager();

        void update_mouse_pos();
        void input_loop();

        // control
        bool m_running = true;

    public:
        static InputManager *instance(CefRefPtr<CefBrowserHost> browser_host = nullptr);

        vec2i get_mouse_position();

        std::thread *get_thread()
        {
            return &m_input_thread;
        }

        bool wait_for_key(int keycode); // block until key is pressed
        bool wait_for_mouse_button(int button, vec2i &mouse_pos);

        void shutdown();
    };
}