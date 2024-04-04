#include "stdafx.h"
#include "Log.h"
#include "GuiTypes.h"
#include "InputParser.h"

namespace gui
{
using namespace types;

#define CheckCoordinatesInRange(coordinate_id, startPosition, endPosition, point) \
    startPosition.coordinate_id <= point.coordinate_id && point.coordinate_id <= endPosition.coordinate_id

struct IShape
{
public:
    virtual size_t GetArea() const = 0;
    virtual size_t GetPerimeter() const = 0;
    // element position means center position of the shape
    virtual bool IsInsideArea(const Point i_elementPosition, const Point i_pointer) const = 0;
    virtual std::string GetTypeName() const = 0;
    virtual std::unique_ptr<IShape> Clone() = 0;

protected:
    template <class _Ty> friend struct std::default_delete;
    IShape() = default;
    virtual ~IShape() = default;
};

struct RectangleShape : public IShape
{
public:
    RectangleShape(Length i_width, Length i_height) : width(i_width), height(i_height)
    {
        assert(i_width.is_valid() && i_height.is_valid());
        if (!i_width.is_valid() || !i_height.is_valid())
        {
            ERROR_LOG("Invalid Value", "Invalid length");
        }
    }
    ~RectangleShape() = default;

    size_t GetArea() const override
    {
        return size_t((width * height).value);
    }

    size_t GetPerimeter() const override
    {
        return size_t(2 * (width + height).value);
    }

    bool IsInsideArea(const Point i_elementPosition, const Point i_pointer) const override
    {
        if (!(i_elementPosition.IsValid() && i_pointer.IsValid()) || !(width.is_valid() && height.is_valid()))
        {
            return false;
        }
        uint16_t halfWidth = width.value / 2;
        uint16_t halfHeight = height.value / 2;
        Point startPosition(i_elementPosition.x - halfWidth, i_elementPosition.y - halfHeight); // top left
        Point endPosition(i_elementPosition.x + halfWidth, i_elementPosition.y + halfHeight); // bottom right
        const bool isXInRange = CheckCoordinatesInRange(x, startPosition, endPosition, i_pointer);
        const bool isYInRange = CheckCoordinatesInRange(y, startPosition, endPosition, i_pointer);
        return isXInRange && isYInRange;
    }

    std::string GetTypeName() const override
    {
        return "RectangleShape";
    }

    std::unique_ptr<IShape> Clone() override
    {
        return std::make_unique<RectangleShape>(width, height);
    }

    Length GetWidth() const
    {
        return width;
    }

    Length GetHeight() const
    {
        return height;
    }

private:
    Length width;
    Length height;
};

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

    utils::Signal<void(), SignalKey> sig_onClick;

protected:
    template <class _Ty> friend struct std::default_delete;
    IButton(const Point i_initialPosition, utils::unique_ref<IShape>&& i_shape) : m_buttonPosition(i_initialPosition), m_shape(std::move(i_shape)) {}
    virtual ~IButton() = default;

    virtual void SimulateClick(float x, float y) = 0;

    Point m_buttonPosition;
    utils::unique_ref<IShape> m_shape;
};

class Button : public IButton
{
public:
    Button(const Point i_initialPosition, utils::unique_ref<IShape>&& i_shape) : IButton(i_initialPosition, std::move(i_shape))
    {
    }

    ~Button() = default;

    void SimulateClick(float x, float y) override
    {
        if (m_shape->IsInsideArea(m_buttonPosition, Point(x, y)))
        {
            utils::Access<IButton::SignalKey>(sig_onClick).Emit();
        }
    }
};

class IGUIBuilder
{
public:
    IGUIBuilder() = default;
    virtual ~IGUIBuilder() = default;

    virtual void reset() = 0;
    virtual void SetShape(std::unique_ptr<IShape>&& i_shape) = 0;
    virtual void SetPosition(const Point& i_position) = 0;
};

class ButtonBuilder : public IGUIBuilder
{
public:
    ButtonBuilder() : m_position() {}
    ~ButtonBuilder() = default;

    void reset() override
    {
        m_button.reset();
        m_shape.reset();
        m_position = Point();
    }

    void SetShape(std::unique_ptr<IShape>&& i_shape) override
    {
        m_shape = std::move(i_shape);
    }

    void SetPosition(const Point& i_position) override
    {
        m_position = i_position;
    }

    std::unique_ptr<IButton> GetResult()
    {
        std::unique_ptr<IButton> button;
        if (m_shape)
        {
            button = std::make_unique<Button>(m_position, m_shape.release());
            reset();
        }
        else
        {
            m_errorString = "Must set shape before constructing!";
        }
        return button;
    }

    std::optional<std::string> GetErrorString() const
    {
        return m_errorString;
    }

private:
    Point m_position;
    std::unique_ptr<IShape> m_shape;
    std::unique_ptr<IButton> m_button;
    std::optional<std::string> m_errorString;
};

class GUIConstructDirector
{
public:
    static void ConstructGUIElement(IGUIBuilder& i_builder, const Point& i_position, std::unique_ptr<IShape>&& i_shape)
    {
        i_builder.reset();
        i_builder.SetPosition(i_position);
        i_builder.SetShape(std::move(i_shape));
    }

    static void ConstructRectangleButton(IGUIBuilder& i_builder, const RectangleShape& i_buttonShape, const Point& i_buttonPosition)
    {
        std::unique_ptr<RectangleShape> buttonShape = std::make_unique<RectangleShape>(i_buttonShape.GetWidth(), i_buttonShape.GetHeight());
        ConstructGUIElement(i_builder, i_buttonPosition, std::move(buttonShape));
    }
};

class IGUIFactory
{
public:
    virtual ~IGUIFactory() = default;
    virtual std::unique_ptr<IButton> CreateButton(const Point& i_buttonPosition, const IShape& i_buttonShape) = 0;
};

class GUIFactory : public IGUIFactory
{
public:
    ~GUIFactory() = default;

    std::unique_ptr<IButton> CreateButton(const Point& i_buttonPosition, const IShape& i_buttonShape) override
    {
        ButtonBuilder buttonBuilder{};
        if (i_buttonShape.GetTypeName() == "RectangleShape")
        {
            const RectangleShape& buttonShape = static_cast<const RectangleShape&>(i_buttonShape);
            GUIConstructDirector::ConstructRectangleButton(buttonBuilder, buttonShape, i_buttonPosition);
        }
        std::unique_ptr<IButton> button = buttonBuilder.GetResult();
        if (button == nullptr)
        {
            ERROR_LOG("GUIFactory", "error: {}", buttonBuilder.GetErrorString().value());
        }
        return std::move(button);
    }
};

} // namespace gui

void OnButtonClicked()
{
    utils::Log::TextFormat textFormat(utils::Log::TextStyle::Bold, { 128, 0, 128 });
    INFO_LOG_WITH_FORMAT(textFormat, "ButtonClicked", "The Button has been clicked!");
}

int main(int argv, char **argc)
{
    using namespace gui;
    utils::Log::s_logThreadMode = utils::MODE::MESSAGE_QUEUE_MT;
    std::unique_ptr<IGUIFactory> guiFactory;
    guiFactory = std::make_unique<GUIFactory>();
    if (guiFactory == nullptr)
    {
        std::terminate();
    }
    std::unique_ptr<IButton> button = guiFactory->CreateButton(Point(20, 100), RectangleShape(100, 50));
    if (button == nullptr)
    {
        std::terminate();
    }
    utils::Connection onClick = button->sig_onClick.Connect(&OnButtonClicked);

    Button* actualButton = dynamic_cast<Button*>(button.get());

    InputParser parser(argv, argc);
    float x = -1;
    float y = -1;
    if (InputParser::position_t pos = parser.HaveInputOptions(InputOptions({"-x"})))
    {
        std::optional<std::string> optX = parser.ExtractValue(pos);
        if (optX.has_value())
        {
            x = atof(optX.value().c_str());
        }
    }

    if (InputParser::position_t pos = parser.HaveInputOptions(InputOptions({"-y"})))
    {
        std::optional<std::string> optY = parser.ExtractValue(pos);
        if (optY.has_value())
        {
            y = atof(optY.value().c_str());
        }
    }

    actualButton->SimulateClick(x, y);
    utils::Log::Wait();
    return 0;
}