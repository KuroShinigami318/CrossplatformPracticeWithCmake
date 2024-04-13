#include "stdafx.h"
#include "Shapes/RectangleShape.h"

#define CheckCoordinatesInRange(coordinate_id, startPosition, endPosition, point) \
    startPosition.coordinate_id <= point.coordinate_id && point.coordinate_id <= endPosition.coordinate_id

namespace gui
{
    RectangleShape::RectangleShape(types::Length i_width, types::Length i_height) : width(i_width), height(i_height)
    {
        assert(i_width.is_valid() && i_height.is_valid());
        if (!i_width.is_valid() || !i_height.is_valid())
        {
            CRASH_PLAIN_MSG("Invalid Shape! width: {}, height: {}", i_width, i_height);
        }
    }

    size_t RectangleShape::GetArea() const
    {
        return size_t((width * height).value);
    }

    size_t RectangleShape::GetPerimeter() const
    {
        return size_t(2 * (width + height).value);
    }

    bool RectangleShape::IsInsideArea(const Point i_elementPosition, const Point i_pointer) const
    {
        if (!(i_elementPosition.IsValid() && i_pointer.IsValid()) || !(width.is_valid() && height.is_valid()))
        {
            return false;
        }
        uint16_t halfWidth = width.value / 2;
        uint16_t halfHeight = height.value / 2;
        Point startPosition(i_elementPosition.x - halfWidth, i_elementPosition.y - halfHeight); // top left
        Point endPosition(i_elementPosition.x + halfWidth, i_elementPosition.y + halfHeight); // bottom right
        const bool isXInRange = CheckCoordinatesInRange(x, startPosition, endPosition, i_pointer);
        const bool isYInRange = CheckCoordinatesInRange(y, startPosition, endPosition, i_pointer);
        return isXInRange && isYInRange;
    }

    std::unique_ptr<IShape> RectangleShape::Clone() const
    {
        return std::make_unique<RectangleShape>(width, height);
    }

    types::Length RectangleShape::GetWidth() const
    {
        return width;
    }

    types::Length RectangleShape::GetHeight() const
    {
        return height;
    }
}