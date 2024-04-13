#include "stdafx.h"
#include "Buttons/ButtonBuilder.h"
#include <Buttons/Button.h>

namespace gui
{
    ButtonBuilder::ButtonBuilder() : m_position() {}

    void ButtonBuilder::reset()
    {
        m_button.reset();
        m_shape.reset();
        m_position = Point();
        m_error.reset();
    }

    void ButtonBuilder::SetShape(std::unique_ptr<IShape>&& i_shape)
    {
        m_shape = std::move(i_shape);
    }

    void ButtonBuilder::SetPosition(const Point& i_position)
    {
        if (!i_position.IsValid())
        {
            m_error = make_error<BuildError>(BuildErrorCode::InvalidPosition, "Invalid position of button! {}", i_position);
            return;
        }
        m_position = i_position;
    }

    ButtonBuilder::BuildResult ButtonBuilder::GetResult()
    {
        std::unique_ptr<IButton> button;
        if (m_error.has_value())
        {
            return *m_error;
        }
        if (m_shape)
        {
            button = std::make_unique<Button>(m_position, m_shape.release());
            reset();
        }
        else
        {
            m_error = make_error<BuildError>(BuildErrorCode::UnsetShape, "Must set shape before constructing!");
            return *m_error;
        }
        return std::move(button);
    }

    std::optional<ButtonBuilder::BuildError> ButtonBuilder::GetError() const
    {
        return m_error;
    }
}