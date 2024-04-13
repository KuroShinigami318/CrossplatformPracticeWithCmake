#pragma once
#include "IGUIBuilder.h"

namespace gui
{
class GUIConstructDirector
{
public:
    static void ConstructGUIElement(IGUIBuilder& i_builder, const Point& i_position, std::unique_ptr<IShape>&& i_shape)
    {
        i_builder.reset();
        i_builder.SetPosition(i_position);
        i_builder.SetShape(std::move(i_shape));
    }

    static void ConstructRectangleButton(IGUIBuilder& i_builder, const IRectangleShape& i_buttonShape, const Point& i_buttonPosition)
    {
        ConstructGUIElement(i_builder, i_buttonPosition, std::move(i_buttonShape.Clone()));
    }
};
}