#pragma once
#include "IGUIFactory.h"
#include "GUIConstructDirector.h"

namespace gui
{
class GUIFactory : public IGUIFactory
{
public:
    ~GUIFactory() = default;

    ButtonBuilder::BuildResult CreateButton(const Point& i_buttonPosition, const IShape& i_buttonShape) override
    {
        ButtonBuilder buttonBuilder{};
        if (const IRectangleShape* buttonShape = dynamic_cast<const IRectangleShape*>(&i_buttonShape))
        {
            GUIConstructDirector::ConstructRectangleButton(buttonBuilder, *buttonShape, i_buttonPosition);
        }
        return buttonBuilder.GetResult();
    }
};
}