#pragma once
#include <vector>
#include <string>
#include <optional>

struct InputOptions
{
	InputOptions(std::vector<std::string>);
	std::vector<std::string> options;
	std::optional<std::string> foundToken;
};

struct InputParser
{
	using tokens_t = std::vector<std::string>;
	using token_t_it = typename tokens_t::iterator;
	using token_t_cit = typename tokens_t::const_iterator;
	struct position_t
	{
		position_t(const InputOptions);
		position_t(const InputOptions, token_t_cit, size_t);
		operator bool() const;
		const InputOptions inputOptions;
		std::optional<token_t_cit> optValueIt;
		size_t position;
	};

	InputParser(std::vector<std::string>);
	InputParser(int argv, char **argc);
	position_t HaveInputOptions(InputOptions);
	position_t HaveInputOptionsWithSeperatTokens(InputOptions&);
	position_t HaveInputOptionsWithoutSeperatTokens(InputOptions&);
	std::optional<std::string> ExtractValue(position_t);
	std::string to_string();
	tokens_t tokens;
	bool isSeperateTokens;
};