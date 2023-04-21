#include "Input/InputManager.hpp"

#include "util/scope_guard.hpp"

namespace WUI
{
    InputManager *InputManager::m_instance = nullptr;
    CefRefPtr<CefBrowserHost> InputManager::m_browser_host = nullptr;

    InputManager *InputManager::instance(CefRefPtr<CefBrowserHost> browser_host)
    {
        if (!m_instance)
        {
            if (browser_host == nullptr)
            {
                DLOG(FATAL) << "InputManager::instance() browser_host == nullptr, first call needs to be with a valid browser_host";
                exit(2);
            }
            m_browser_host = browser_host;
            m_instance = new InputManager();
        }
        return m_instance;
    }

    InputManager::InputManager()
    {
        // get communication from gamemanager

        assert(al_install_mouse() && "could not install mouse driver");
        assert(al_install_keyboard() && "could not install keyboard driver");

        assert(!m_input_thread.joinable());

        m_InputManager_event_queue = al_create_event_queue();

        al_init_user_event_source(&m_InputManager_event_source);

        al_register_event_source(m_InputManager_event_queue, al_get_mouse_event_source());
        al_register_event_source(m_InputManager_event_queue, al_get_keyboard_event_source());

        m_input_thread = std::thread([=]() -> void
                                     { this->input_loop(); });
    }

    static void convertMouseEvent(ALLEGRO_EVENT &event, CefMouseEvent &cef_event)
    {
        cef_event.x = event.mouse.x;
        cef_event.y = event.mouse.y;
        cef_event.modifiers = 0; // TODO mouse input modifiers
    }

    [[maybe_unused]] static void convertKeyEvent(ALLEGRO_EVENT &event, CefKeyEvent &cef_event)
    {
        cef_event.windows_key_code = event.keyboard.keycode;
        cef_event.native_key_code = event.keyboard.keycode;
        cef_event.modifiers = 0; // TODO key input modifiers
        cef_event.is_system_key = false;
    }

    void InputManager::input_loop()
    {

        static CefMouseEvent cef_mouse_event;
        static CefKeyEvent cef_key_event;

        // inject user-input by calling - non-trivial for non-windows - checkout the cefclient source and the platform specific cpp, like cefclient_osr_widget_gtk.cpp for linux
        // browser->GetHost()->SendKeyEvent(...);
        // browser->GetHost()->SendMouseMoveEvent(...);
        // browser->GetHost()->SendMouseClickEvent(...);
        // browser->GetHost()->SendMouseWheelEvent(...);

        while (m_running)
        {
            ALLEGRO_EVENT event;

            // Fetch the event (if one exists)
            al_wait_for_event(m_InputManager_event_queue, &event);

            // Handle the event

            switch (event.type)
            {
            case ALLEGRO_EVENT_MOUSE_AXES:

                l_mouse_state.lock();
                m_mouse_state.x = event.mouse.x;
                m_mouse_state.y = event.mouse.y;
                l_mouse_state.unlock();

                convertMouseEvent(event, cef_mouse_event);

                m_browser_host->SendMouseMoveEvent(cef_mouse_event, false);
                // DLOG(INFO) << "mouse moved to " << event.mouse.x << " " << event.mouse.y;

                break;
            case ALLEGRO_EVENT_KEY_DOWN:
                DLOG(INFO) << "key pressed " << al_keycode_to_name(event.keyboard.keycode);

                // TODO maybe some key events want to be forwarded to the UI
                // this could be necessary but we can't check if the key is used by the ui
                // so, doing it the other way. check if allegro consumes it and forward it by a on-use basis
                // TODO, we just ignore it

                // convertKeyEvent(event, cef_key_event);
                // m_browser_host->SendKeyEvent(cef_key_event);

                al_emit_user_event(&m_InputManager_event_source, &event, nullptr);

                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                DLOG(INFO) << "mouse DOWN Button " << (event.mouse.button == 1 ? "left" : "right") << "(" << event.mouse.button << ">2 = other) @ " << event.mouse.x << " " << event.mouse.y;

                convertMouseEvent(event, cef_mouse_event);
                m_browser_host->SendMouseClickEvent(cef_mouse_event, event.mouse.button == 1 ? MBT_LEFT : MBT_RIGHT, false, 1);
                al_emit_user_event(&m_InputManager_event_source, &event, nullptr);

                // TODO some kind of system to differenciate if the UI consumed the event or not?
                // possibly only solvable JS side.
                // allegro -> UI -> consumes -> end event
                // allegro -> UI -> doesn't consume -> back to allegro via a binding

                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
                DLOG(INFO) << "mouse UP Button " << (event.mouse.button == 1 ? "left" : "right") << "(" << event.mouse.button << ">2 = other) @ " << event.mouse.x << " " << event.mouse.y;

                convertMouseEvent(event, cef_mouse_event);
                m_browser_host->SendMouseClickEvent(cef_mouse_event, event.mouse.button == 1 ? MBT_LEFT : MBT_RIGHT, true, 1);
                al_emit_user_event(&m_InputManager_event_source, &event, nullptr);

                // TODO some kind of system to differenciate if the UI consumed the event or not?
                // possibly only solvable JS side.
                // allegro -> UI -> consumes -> end event
                // allegro -> UI -> doesn't consume -> back to allegro via a binding

                break;

            case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY:
            case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
            case ALLEGRO_EVENT_MOUSE_WARPED:
                // ignore since they have permission problems on ubuntu/kde
                break;

            case ALLEGRO_EVENT_KEY_CHAR:
            case ALLEGRO_EVENT_KEY_UP:
                // ignore all type events or key up events
                al_emit_user_event(&m_InputManager_event_source, &event, nullptr);

                break;

            default:
                DLOG(INFO) << "[Input] event received: " << event.type;
                break;
            }
        }
        DLOG(INFO) << ("[Input] exited");
        return;
    }

    vec2i InputManager::get_mouse_position()
    {
        l_mouse_state.lock();
        auto ret = m_mouse_state;
        l_mouse_state.unlock();
        return ret;
    }

    bool InputManager::wait_for_key(int keycode)
    {
        auto queue = al_create_event_queue();
        al_register_event_source(queue, &m_InputManager_event_source);
        bool exit = false;
        while (!exit)
        {
            ALLEGRO_EVENT event;
            al_wait_for_event(queue, &event);
            if (event.type == ALLEGRO_EVENT_KEY_DOWN)
            {
                exit = (event.keyboard.keycode == keycode);
            }
        }
        al_unregister_event_source(queue, &m_InputManager_event_source);
        al_destroy_event_queue(queue);
        return true;
    }

    bool InputManager::wait_for_mouse_button(int button, vec2i &mouse_pos)
    {
        auto queue = al_create_event_queue();
        al_register_event_source(queue, &m_InputManager_event_source);
        bool exit = false;
        while (!exit)
        {
            ALLEGRO_EVENT event;
            al_wait_for_event(queue, &event);
            if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
            {
                exit = (event.mouse.button & button);
                if (exit)
                {
                    mouse_pos = {event.mouse.x, event.mouse.y};
                }
            }
        }
        al_unregister_event_source(queue, &m_InputManager_event_source);
        al_destroy_event_queue(queue);

        return true;
    }

    void InputManager::shutdown()
    {
        DLOG(INFO) << ("[Input] shutting down");
        m_running = false;
        ALLEGRO_EVENT ev = {};
        ev.user.data1 = (int)1;
        ev.type = 1;
        al_emit_user_event(&m_InputManager_event_source, &ev, nullptr);
    }

}