#include "RenderHandler.hpp"

#include <allegro5/allegro_primitives.h>
namespace WUI
{

#define FULL_REDRAW 1

    RenderHandler::RenderHandler(const int &FPS,
                                 const int &width,
                                 const int &height)
    {
        if (!al_is_system_installed())
        {

            DLOG(WARNING) << "Allegro not installed on Renderer start";
            al_init();
            al_init_primitives_addon();
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

        m_background_color = CefColorSetARGB(255, 255, 0, 0);
    }

    RenderHandler::~RenderHandler()
    {
        al_destroy_display(m_display);
        al_destroy_event_queue(m_event_queue);
    }

    void RenderHandler::renderLoop()
    {
        // Start the timer
        al_start_timer(m_timer);
        m_running = true;

        // Game loop
        while (m_running)
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
                    m_running = false;
                    break;
                default:
                    DLOG(INFO) << "Unsupported event received: " << event.type;
                    break;
                }
            }

            // Check if we need to redraw
            if (m_redraw_pending && al_is_event_queue_empty(m_event_queue))
            {
                if (IsTransparent())
                {
                    al_clear_to_color(al_map_rgba(0, 0, 0, 0));
                }
                else
                {
                    al_clear_to_color(al_map_rgba(CefColorGetR(m_background_color),
                                                  CefColorGetG(m_background_color),
                                                  CefColorGetB(m_background_color),
                                                  255));
                }

                // Redraw

                static double delta_s = 0;

                {
                    static auto last_delta_time_point = std::chrono::high_resolution_clock::now();
                    auto end = std::chrono::high_resolution_clock::now();
                    delta_s = std::chrono::duration<double, std::milli>(end - last_delta_time_point).count() / 1000; // why is chrono like this -.-
                    last_delta_time_point = end;
                }

                m_l_renderables.lock();
                for (auto &renderable : m_renderables)
                {
                    renderable->render(al_get_display_width(m_display),
                                       al_get_display_height(m_display), delta_s);
                }
                m_l_renderables.unlock();

                // draw UI

                if (m_l_osr_buffer_lock.try_lock())
                {
                    // al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
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

        // teardown
        al_destroy_timer(m_timer);
        al_destroy_display(m_display);
        al_destroy_bitmap(m_osr_buffer);
        al_destroy_event_queue(m_event_queue);
    }

    // CefRenderHandler interface
    void RenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
    {
        rect = CefRect(0, 0, al_get_display_width(m_display), al_get_display_height(m_display));
    }

    void RenderHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height)
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

        m_l_osr_buffer_lock.lock();

#if FULL_REDRAW
        // DLOG(INFO) << "Full  redraw";

        const int size = width * height * 4;

        auto locked_region = al_lock_bitmap(m_osr_buffer, ALLEGRO_PIXEL_FORMAT_RGBA_8888, ALLEGRO_LOCK_WRITEONLY);
        if (!locked_region)
        {
            DLOG(FATAL) << "Failed to lock bitmap";
            exit(1);
        }

        // Data copied in is in format BGRA
        // paint the region in a random color

        memcpy(locked_region->data, (void *)((size_t)buffer_rgba), size);

        m_l_osr_buffer_lock.unlock();
#else
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
#endif

        al_unlock_bitmap(m_osr_buffer);
    }

    // CefBase interface

    ALLEGRO_DISPLAY *RenderHandler::getDisplay() const
    {
        return m_display;
    }

    void RenderHandler::shutdown()
    {
        DLOG(INFO) << ("[Renderer] shutting down");

        m_running = false;
    }

}