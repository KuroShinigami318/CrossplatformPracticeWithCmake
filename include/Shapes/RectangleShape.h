#pragma once
#include "IShape.h"

namespace gui
{
struct RectangleShape : public IRectangleShape
{
public:
    RectangleShape(types::Length i_width, types::Length i_height);
    ~RectangleShape() = default;

    size_t GetArea() const override;
    size_t GetPerimeter() const override;
    bool IsInsideArea(const Point i_elementPosition, const Point i_pointer) const override;
    std::unique_ptr<IShape> Clone() const override;
    types::Length GetWidth() const override;
    types::Length GetHeight() const override;

private:
    types::Length width;
    types::Length height;
};
}