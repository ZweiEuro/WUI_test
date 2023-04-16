#pragma once

#include "include/cef_client.h"

#include "RenderHandler.hpp"

// Client actually handling the html internals
class BrowserClient : public CefClient
{
public:
    BrowserClient(RenderHandler *renderHandler)
        : m_renderHandler(renderHandler)
    {
        ;
    }

    virtual CefRefPtr<CefRenderHandler> GetRenderHandler()
    {
        return m_renderHandler;
    }

    CefRefPtr<CefRenderHandler> m_renderHandler;

    IMPLEMENT_REFCOUNTING(BrowserClient);
};