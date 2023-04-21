#pragma once
#include <cstddef>
namespace WUI
{
    class Renderable
    {
    public:
        virtual void render(const size_t displayWidth, const size_t displayHeight) = 0;
    };
}