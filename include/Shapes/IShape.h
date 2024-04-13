#pragma once
#include "GuiTypes.h"
#include <memory>

namespace gui
{
struct IShape
{
public:
    virtual size_t GetArea() const = 0;
    virtual size_t GetPerimeter() const = 0;
    // element position means center position of the shape
    virtual bool IsInsideArea(const Point i_elementPosition, const Point i_pointer) const = 0;
    virtual std::unique_ptr<IShape> Clone() const = 0;

protected:
    template <class _Ty> friend struct std::default_delete;
    IShape() = default;
    virtual ~IShape() = default;
};

struct IRectangleShape : public IShape
{
public:
    virtual ~IRectangleShape() = default;
    virtual types::Length GetWidth() const = 0;
    virtual types::Length GetHeight() const = 0;
};
}