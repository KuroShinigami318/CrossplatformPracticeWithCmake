#pragma once
#include "Shapes/IShape.h"
#include "unique_ref.h"

namespace gui
{
class IButton
{
protected:
    struct SignalKey;

public:
    virtual IShape* GetShape() const
    {
        return m_shape.get();
    }

    virtual Point GetObjectPosition() const
    {
        return m_buttonPosition;
    }

    utils::Signal<void(), SignalKey> sig_onAction;

protected:
    template <class _Ty> friend struct std::default_delete;
    IButton(const Point i_initialPosition, utils::unique_ref<IShape>&& i_shape) : m_buttonPosition(i_initialPosition), m_shape(std::move(i_shape)) {}
    virtual ~IButton() = default;

    virtual void SimulateClick(const Point point) = 0;

    Point m_buttonPosition;
    utils::unique_ref<IShape> m_shape;
};
}