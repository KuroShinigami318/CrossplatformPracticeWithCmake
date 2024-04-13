#include "stdafx.h"
#include "Log.h"
#include "Buttons/Button.h"
#include "GUIFactory.h"
#include "Shapes/RectangleShape.h"
#include "InputParser.h"

void OnButtonClicked()
{
    utils::Log::TextFormat textFormat(utils::Log::TextStyle::Bold, { 128, 0, 128 });
    INFO_LOG_WITH_FORMAT(textFormat, "ButtonClicked", "The Button has been clicked!");
}

float ParseCoordinatorValue(InputParser& parser, const InputOptions& options)
{
    if (InputParser::position_t pos = parser.HaveInputOptions(options))
    {
        return atof(parser.ExtractValue(pos).c_str());
    }
    return -1;
}

int main(int argv, char **argc)
{
    using namespace gui;
    utils::Log::s_logThreadMode = utils::MODE::MESSAGE_QUEUE_MT;
    std::unique_ptr<IGUIFactory> guiFactory;
    guiFactory = std::make_unique<GUIFactory>();
    if (guiFactory == nullptr)
    {
        CRASH();
    }
    ButtonBuilder::BuildResult buildResult = guiFactory->CreateButton(Point(20, 100), RectangleShape(100, 50));
    if (buildResult.isErr())
    {
        CRASH_PLAIN_MSG("Error while create button: {}", buildResult.unwrapErr());
    }
    buildResult.storage().template get<std::unique_ptr<gui::IButton>>()->sig_onAction.Connect(&OnButtonClicked).Detach();

    Button* actualButton = dynamic_cast<Button*>(buildResult.storage().template get<std::unique_ptr<gui::IButton>>().get());

    InputParser parser(argv, argc);
    float x = ParseCoordinatorValue(parser, InputOptions({ "-x" }));
    float y = ParseCoordinatorValue(parser, InputOptions({ "-y" }));
    actualButton->SimulateClick({ x, y });
    utils::Log::Wait();
    return 0;
}