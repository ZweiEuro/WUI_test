#pragma once

#include <allegro5/allegro.h>
#include <stdio.h>
#include <atomic>

#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include <mutex>

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

    // OSR buffer
    ALLEGRO_BITMAP *m_osr_buffer = NULL;
    std::mutex m_l_osr_buffer_lock;

public:
    RenderHandler(const int &FPS = BASE_FPS,
                  const int &width = BASE_WIDTH,
                  const int &height = BASE_HEIGHT)
    {
        if (!al_is_system_installed())
        {

            DLOG(WARNING) << "Allegro not installed on Renderer start";
            al_init();
        }

        m_display = al_create_display(width, height);

        al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP); // use memory bitmap for OSR buffer

        m_osr_buffer = al_create_bitmap(BASE_WIDTH, BASE_HEIGHT);

        if (!m_display || !m_osr_buffer)
        {
            DLOG(FATAL) << "Failed to create display or OSR bitmap buffer";
            exit(1);
        }

        // clear entire bitmap to white
        auto locked_region = al_lock_bitmap(m_osr_buffer, ALLEGRO_PIXEL_FORMAT_RGBA_8888, ALLEGRO_LOCK_WRITEONLY);
        memset(locked_region->data, 0, width * height * locked_region->pixel_size);
        al_unlock_bitmap(m_osr_buffer);

        m_timer = al_create_timer(1.0 / FPS);
        if (!m_timer)
        {
            DLOG(FATAL) << "Failed to create timer.";
            exit(1);
        }

        // Create the event queue
        m_event_queue = al_create_event_queue();
        if (!m_event_queue)
        {
            DLOG(FATAL) << "Failed to create event queue.";
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
                    DLOG(INFO) << "Unsupported event received: " << event.type;
                    break;
                }
            }

            // Check if we need to redraw
            if (m_redraw_pending && al_is_event_queue_empty(m_event_queue))
            {
                // Redraw

                if (m_l_osr_buffer_lock.try_lock())
                {
                    al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
                    al_draw_bitmap(m_osr_buffer, 0, 0, 0);
                    m_l_osr_buffer_lock.unlock();
                }
                else
                {
                    DLOG(WARNING) << "OSR buffer locked, skipping redraw";
                }

                al_flip_display();
                m_redraw_pending = false;
            }
            CefDoMessageLoopWork();
        }
    }

    // CefRenderHandler interface
public:
    virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
    {
        rect = CefRect(0, 0, al_get_display_width(m_display), al_get_display_height(m_display));
    }

    virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height)
    {

        if (dirtyRects.size() != 1)
        {
            // unclear how the buffer is organized when more than 1 rect needs to be redrawn

            DLOG(FATAL) << "redrawing " << dirtyRects.size() << " rects, which is not yet implemented";
        }

        // convert buffer format to BGRA to RGBA
        auto buffer_rgba = new uint8_t[width * height * 4];
        memset(buffer_rgba, 0, width * height * 4);

        for (int i = 0; i < width * height; i++)
        {

            buffer_rgba[i * 4 + 0] = ((uint8_t *)buffer)[i * 4 + 3];
            buffer_rgba[i * 4 + 1] = ((uint8_t *)buffer)[i * 4 + 0];
            buffer_rgba[i * 4 + 2] = ((uint8_t *)buffer)[i * 4 + 1];
            buffer_rgba[i * 4 + 3] = ((uint8_t *)buffer)[i * 4 + 2];

            /*
            B  -> A
            G  -> R
            R  -> G
            A  -> B

            */
        }

        DLOG(INFO) << "want lock";
        m_l_osr_buffer_lock.lock();
        DLOG(INFO) << "locked";
        for (auto rect : dirtyRects)
        {
            // TODO create the correct color buffer here and only for as large as the dirty rect needs it
            const size_t offset = (rect.y * width + rect.x) * 4;
            const int size = rect.width * rect.height * 4;

            DLOG(INFO) << "dirty Rect: " << rect.x << " " << rect.y << " " << rect.width << " " << rect.height << " " << width << " " << height;

            // lock the region
            auto locked_region = al_lock_bitmap_region(m_osr_buffer, rect.x, rect.y, rect.width, rect.height, ALLEGRO_PIXEL_FORMAT_RGBA_8888, ALLEGRO_LOCK_WRITEONLY);
            if (!locked_region)
            {
                DLOG(FATAL) << "Failed to lock region"
                            << "dirty Rect: " << rect.x << " " << rect.y << " " << rect.width << " " << rect.height;
                exit(1);
            }

            // Data copied in is in format BGRA
            // paint the region in a random color

            memcpy(locked_region->data, (void *)((size_t)buffer_rgba + offset), size);

            m_l_osr_buffer_lock.unlock();
        }
        al_unlock_bitmap(m_osr_buffer);
    }

    // CefBase interface

    ALLEGRO_DISPLAY *getDisplay() const
    {
        return m_display;
    }

public:
    IMPLEMENT_REFCOUNTING(RenderHandler);
};
