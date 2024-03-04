#include "InputParser.h"

InputOptions::InputOptions(std::vector<std::string> i_options)
	: options(i_options)
{
}

InputParser::InputParser(std::vector<std::string> i_arguments)
	: tokens(i_arguments), isSeperateTokens(false)
{
}

InputParser::InputParser(int argv, char** argc) : isSeperateTokens(true)
{
	for (int i = 0; i < argv; ++i)
	{
		tokens.push_back(argc[i]);
	}
}

InputParser::position_t::position_t(const InputOptions i_inputOptions)
	: inputOptions(i_inputOptions), optValueIt(std::nullopt), position(std::string::npos)
{
}

InputParser::position_t::position_t(const InputOptions i_inputOptions, token_t_cit i_valueIt, size_t i_position)
	: inputOptions(i_inputOptions), optValueIt(i_valueIt), position(i_position)
{
}

InputParser::position_t::operator bool() const
{
	return optValueIt.has_value() && inputOptions.foundToken.has_value() && position != std::string::npos;
}

InputParser::position_t InputParser::HaveInputOptions(InputOptions i_inputOptions)
{
	if (isSeperateTokens)
	{
		return HaveInputOptionsWithSeperatTokens(i_inputOptions);
	}
	else
	{
		return HaveInputOptionsWithoutSeperatTokens(i_inputOptions);
	}
}

InputParser::position_t InputParser::HaveInputOptionsWithSeperatTokens(InputOptions& i_inputOptions)
{
	for (token_t_cit tokenIt = tokens.cbegin(); tokenIt != tokens.cend(); ++tokenIt)
	{
		for (const std::string& option : i_inputOptions.options)
		{
			if (tokenIt->find(option) != std::string::npos && tokenIt + 1 != tokens.end())
			{
				i_inputOptions.foundToken = option;
				return position_t(i_inputOptions, tokenIt + 1, 0);
			}
		}
	}
	return position_t(i_inputOptions);
}

InputParser::position_t InputParser::HaveInputOptionsWithoutSeperatTokens(InputOptions& i_inputOptions)
{
	for (token_t_cit tokenIt = tokens.cbegin(); tokenIt != tokens.cend(); ++tokenIt)
	{
		for (const std::string& option : i_inputOptions.options)
		{
			if (size_t pos = tokenIt->find(option); pos != std::string::npos)
			{
				i_inputOptions.foundToken = option;
				return position_t(i_inputOptions, tokenIt, pos);
			}
		}
	}
	return position_t(i_inputOptions);
}

std::optional<std::string> InputParser::ExtractValue(position_t i_pos)
{
	if (!i_pos)
	{
		return std::nullopt;
	}

	std::string value;
	if (isSeperateTokens)
	{
		value = *i_pos.optValueIt.value();
	}
	else
	{
		value = i_pos.optValueIt.value()->substr(i_pos.position + i_pos.inputOptions.foundToken.value().size());
	}
	tokens.erase(i_pos.optValueIt.value());
	return std::move(value);
}

std::string InputParser::to_string()
{
	std::string listStr;
	for (auto cmdIt = tokens.cbegin(); cmdIt != tokens.cend(); ++cmdIt)
	{
		listStr += *cmdIt + (cmdIt == tokens.cend() - 1 ? "." : ", ");
	}
	return listStr;
}