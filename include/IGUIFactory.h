#pragma once
#include "Buttons/ButtonBuilder.h"

namespace gui
{
class IGUIFactory
{
public:
    virtual ~IGUIFactory() = default;
    virtual ButtonBuilder::BuildResult CreateButton(const Point& i_buttonPosition, const IShape& i_buttonShape) = 0;
};
}