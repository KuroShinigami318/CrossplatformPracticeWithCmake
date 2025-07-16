#pragma once
#include "IButton.h"
#include "IGUIBuilder.h"
#include "result.h"
#include <optional>
#include <memory>

namespace gui
{
class ButtonBuilder : public IGUIBuilder
{
public:
    DeclareInnerScopedEnum(BuildErrorCode, uint8_t, InvalidPosition, UnsetShape);
    using BuildError = utils::Error<BuildErrorCode>;
    using BuildResult = utils::Result<std::unique_ptr<IButton>, BuildError>;

public:
    ButtonBuilder();
    ~ButtonBuilder() = default;

    void reset() override;
    void SetShape(std::unique_ptr<IShape>&& i_shape) override;
    void SetPosition(const Point& i_position) override;
    BuildResult GetResult();
    std::optional<BuildError> GetError() const;

private:
    Point m_position;
    std::unique_ptr<IShape> m_shape;
    std::unique_ptr<IButton> m_button;
    std::optional<BuildError> m_error;
};
DefineScopeEnumOperatorImpl(BuildErrorCode, ButtonBuilder);
}