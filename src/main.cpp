#if defined(_WIN32) || defined(WIN32)
#include <winapifamily.h>
#endif
#include "common-utils.h"
#include "Log.h"

struct Point
{
public:
    Point() : Point(0.0f, 0.0f) {}
    Point(float i_x, float i_y) : x(i_x), y(i_y) {}
    float x, y;
};

#define CheckCoordinatesInRange(coordinate_id, startPosition, endPosition, point) \
    startPosition.coordinate_id <= point.coordinate_id && point.coordinate_id <= endPosition.coordinate_id

struct IShape
{
public:
    virtual size_t GetArea() const = 0;
    virtual size_t GetPerimeter() const = 0;
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
    RectangleShape(uint16_t i_width, uint16_t i_height) : width(i_width), height(i_height)
    {
    }
    ~RectangleShape() = default;

    virtual size_t GetArea() const override
    {
        return size_t(width * height);
    }

    virtual size_t GetPerimeter() const override
    {
        return size_t(2 * (width + height));
    }

    virtual std::string GetTypeName() const override
    {
        return "RectangleShape";
    }

    virtual std::unique_ptr<IShape> Clone() override
    {
        return std::make_unique<RectangleShape>(width, height);
    }

    uint16_t GetWidth() const
    {
        return width;
    }

    uint16_t GetHeight() const
    {
        return height;
    }

private:
    uint16_t width;
    uint16_t height;
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

    utils::Signal<void(float, float), SignalKey> sig_onClick;

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

    virtual void SimulateClick(float x, float y) override
    {
        // todo
        // check if click pointer is inside area of Button then Emit Action Onclick
        if (false)
        {
            utils::Access<IButton::SignalKey>(sig_onClick).Emit(x, y);
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

class WinGUIFactory : public IGUIFactory
{
public:
    ~WinGUIFactory() = default;

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

void OnButtonClicked(float x, float y)
{
    INFO_LOG("ButtonClicked", "tap position: x: {} - y: {}", x, y);
}

int main()
{
    std::unique_ptr<IGUIFactory> guiFactory;
#if defined(WINAPI_FAMILY)
    guiFactory = std::make_unique<WinGUIFactory>();
#endif
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

    std::this_thread::sleep_for(std::chrono::seconds(1)); // click after 1s
    // simulate a click here
    Button* actualButton = dynamic_cast<Button*>(button.get());
    actualButton->SimulateClick(20, 100);
    utils::Log::Wait();
    system("pause");
    return 0;
}