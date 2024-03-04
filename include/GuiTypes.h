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
}