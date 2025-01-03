#pragma once
#include "types.h"

namespace gui
{
namespace types
{
DeclareConsecutiveType(Coordinate, float, -1.f, UNDERLYING_TYPE_MAX(float), 0.f);
DeclareConsecutiveType(Length, uint16_t, -1);
} // namespace types
} // namespace gui

namespace gui
{
struct Point
{
public:
    Point() : Point(0.0f, 0.0f) {}
    Point(types::Coordinate i_x, types::Coordinate i_y) : x(i_x), y(i_y) {}
    bool IsValid() const
    {
        return x.is_valid() && y.is_valid();
    }
    types::Coordinate x, y;
};

static std::string to_string(const gui::Point point)
{
    std::ostringstream ss;
    ss << "x: " << point.x << ", y: " << point.y;
    return std::move(ss).str();
}
}

MAKE_FORMATTABLE(gui::Point, std::string, to_string);