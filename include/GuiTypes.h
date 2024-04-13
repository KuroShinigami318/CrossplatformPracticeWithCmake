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

namespace std
{
template <typename T, typename CharT>
struct formatter;

template <typename CharT>
struct formatter<gui::Point, CharT> : formatter<std::string, CharT>
{
    template <typename FormatContext>
    auto format(gui::Point point, FormatContext& ctx) const
    {
        return formatter<std::string, CharT>::format(to_string(point), ctx);
    }
};
}

namespace utils
{
template <typename T, typename CharT>
struct Formatter;

template <typename CharT>
struct Formatter<gui::Point, CharT>
{
    template <typename _FormatContext>
    auto Format(gui::Point point, _FormatContext& ctx) const
    {
        return ctx.parseContext.template Parse<std::string>(to_string(point));
    }
};
}