#pragma once

#include <allegro5/allegro.h>
#include <stdio.h>
#include <atomic>

#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include <mutex>

namespace WUI
{
    const size_t BASE_FPS = 60;
    const size_t BASE_WIDTH = 640;
    const size_t BASE_HEIGHT = 480;

    class RenderHandler : public CefRenderHandler
    {
    private:
        // Required always
        ALLEGRO_DISPLAY *m_display = NULL;

        // Required for rendering
        ALLEGRO_EVENT_QUEUE *m_event_queue = NULL; // Display event loop
        ALLEGRO_TIMER *m_timer = NULL;             // rerender timer

        // Asynchronous control:
        std::atomic<bool> m_renderloop_running = false;
        std::atomic<bool> m_redraw_pending = false;

        // OSR buffer
    private:
        ALLEGRO_BITMAP *m_osr_buffer = NULL;
        std::mutex m_l_osr_buffer_lock;
        cef_color_t m_background_color = 0; // if alpha is 0 then it is transparent

    public:
        RenderHandler(const int &FPS = BASE_FPS,
                      const int &width = BASE_WIDTH,
                      const int &height = BASE_HEIGHT);
        ~RenderHandler();

        // FrameListener interface

        void renderLoop();
        ALLEGRO_DISPLAY *getDisplay() const;

        // CefRenderHandler interface
    public: // OSR CEF stuff
        virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect);

        virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height);
        // CefBase interface

        inline bool IsTransparent() const
        {
            return CefColorGetA(m_background_color) == 0;
        }

        // needed for ref counting
        IMPLEMENT_REFCOUNTING(RenderHandler);
    };

}
