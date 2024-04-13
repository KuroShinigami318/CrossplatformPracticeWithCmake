#pragma once
#include "IButton.h"

namespace gui
{
class Button : public IButton
{
public:
    Button(const Point i_initialPosition, utils::unique_ref<IShape>&& i_shape) : IButton(i_initialPosition, std::move(i_shape))
    {
    }

    ~Button() = default;

    void SimulateClick(const Point point) override
    {
        if (m_shape->IsInsideArea(m_buttonPosition, point))
        {
            utils::Access<IButton::SignalKey>(sig_onAction).Emit();
        }
    }
};
}