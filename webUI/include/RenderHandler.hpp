#pragma once

#include <allegro5/allegro.h>
#include <stdio.h>
#include <atomic>

#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>

#define BASE_FPS 60
#define BASE_WIDTH 640
#define BASE_HEIGHT 480

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

public:
    RenderHandler(const int &FPS = BASE_FPS,
                  const int &width = BASE_WIDTH,
                  const int &height = BASE_HEIGHT)
    {
        if (!al_is_system_installed())
        {
            fprintf(stderr, "Allegro not installed on Renderer start\n");
            al_init();
        }

        m_display = al_create_display(width, height);
        if (!m_display)
        {
            fprintf(stderr, "Failed to create display.\n");
            exit(1);
        }

        m_timer = al_create_timer(1.0 / FPS);
        if (!m_timer)
        {
            fprintf(stderr, "Failed to create timer.\n");
            exit(1);
        }

        // Create the event queue
        m_event_queue = al_create_event_queue();
        if (!m_event_queue)
        {
            fprintf(stderr, "Failed to create event queue.");
            exit(1);
        }

        // Register event sources
        al_register_event_source(m_event_queue, al_get_display_event_source(m_display));
        al_register_event_source(m_event_queue, al_get_timer_event_source(m_timer));

        // Display a black screen, clear the screen once
        al_clear_to_color(al_map_rgb(0, 0, 0));
        al_flip_display();
    }

    ~RenderHandler()
    {
        al_destroy_display(m_display);
        al_destroy_event_queue(m_event_queue);
    }

    // FrameListener interface
public:
    void renderLoop()
    {
        // Start the timer
        al_start_timer(m_timer);
        m_renderloop_running = true;

        // Game loop
        while (m_renderloop_running)
        {
            ALLEGRO_EVENT event;
            ALLEGRO_TIMEOUT timeout;

            // Initialize timeout
            al_init_timeout(&timeout, 0.06);

            // Fetch the event (if one exists)
            bool get_event = al_wait_for_event_until(m_event_queue, &event, &timeout);

            // Handle the event
            if (get_event)
            {
                switch (event.type)
                {
                case ALLEGRO_EVENT_TIMER:
                    m_redraw_pending = true;
                    break;
                case ALLEGRO_EVENT_DISPLAY_CLOSE:
                    m_renderloop_running = false;
                    break;
                default:
                    fprintf(stderr, "Unsupported event received: %d\n", event.type);
                    break;
                }
            }

            // Check if we need to redraw
            if (m_redraw_pending && al_is_event_queue_empty(m_event_queue))
            {
                // Redraw
                if ((int)al_get_time() % 2 == 0)
                {
                    al_clear_to_color(al_map_rgb(255, 0, 0));
                }
                else
                {
                    al_clear_to_color(al_map_rgb(0, 255, 0));
                }

                al_flip_display();
                m_redraw_pending = false;
            }
            CefDoMessageLoopWork();
        }
    }

    // CefRenderHandler interface
public:
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
    {
        rect = CefRect(0, 0, al_get_display_width(m_display), al_get_display_height(m_display));
    }

    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height)
    {

        auto bitmap = al_get_backbuffer(m_display);
        al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY);
        memcpy(bitmap, buffer, width * height * 4);
        al_unlock_bitmap(bitmap);
    }

    // CefBase interface
public:
    IMPLEMENT_REFCOUNTING(RenderHandler);
};
