#include "stdafx.h"
#include "Log.h"
#include "Buttons/Button.h"
#include "GUIFactory.h"
#include "Shapes/RectangleShape.h"
#include "InputParser.h"
#include <flag_set.h>
#include <set>

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

ENUM_DEFINE(test, size_t,
	abc = 1 << 0,
	x = 1 << 1,
	a = 1 << 2,
	d = 1 << 3,
	e = 1 << 4,
	f = 1 << 5,
	g = 1 << 6,
	h = 1 << 7,
	y = 1 << 8,
	z = 1 << 9
);

int main(int argc, char **argv)
{
    utils::flag_set<test> flags;
    flags.set(test::abc);
    flags.set(test::a);
    flags.flip(test::x);
	std::vector<test> flagsVector = { test::a, test::abc };
	std::unordered_set<test> flagsUnorderedSet = { test::a, test::abc };
	std::set<test> flagsSet = { test::a, test::abc };
	std::list<test> flagsList = { test::a, test::abc };
    size_t combinedFlags = flags.combine(test::a, test::abc);
    ASSERT(flags.test_all(test::x, test::abc));
    ASSERT(flags.test_all(combinedFlags));
    ASSERT(flags.test_all(flagsVector));
    ASSERT(flags.test_all(flagsUnorderedSet));
    ASSERT(flags.test_all(flagsSet));
    ASSERT(flags.test_all(flagsList));
    using namespace gui;
    using ButtonT = utils::details::ResultOkType<ButtonBuilder::BuildResult>::type;
    std::unique_ptr<IGUIFactory> guiFactory = std::make_unique<GUIFactory>();
    ButtonBuilder::BuildResult buildResult = guiFactory->CreateButton(Point(20, 100), RectangleShape(100, 50));
    if (buildResult.isErr())
    {
        CRASH_PLAIN_MSG("Error while create button: {}", buildResult.unwrapErr());
    }
    buildResult.storage().get<ButtonT>()->sig_onAction.Connect(&OnButtonClicked).Detach();

    Button* actualButton = dynamic_cast<Button*>(buildResult.storage().get<ButtonT>().get());

    InputParser parser(argc, argv);
    float x = ParseCoordinatorValue(parser, InputOptions({ "-x" }));
    float y = ParseCoordinatorValue(parser, InputOptions({ "-y" }));
    actualButton->SimulateClick({ x, y });
    utils::Log::Wait();
    return 0;
}