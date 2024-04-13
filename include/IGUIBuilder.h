#pragma once
#include "Shapes/IShape.h"
#include <memory>

namespace gui
{
class IGUIBuilder
{
public:
    IGUIBuilder() = default;
    virtual ~IGUIBuilder() = default;

    virtual void reset() = 0;
    virtual void SetShape(std::unique_ptr<IShape>&& i_shape) = 0;
    virtual void SetPosition(const Point& i_position) = 0;
};
}