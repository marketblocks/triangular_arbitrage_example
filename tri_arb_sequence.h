#pragma once

#include "trading/tradable_pair.h"
#include "trading/trading_constants.h"

using namespace cb;

class sequence_step
{
private:
	tradable_pair _pair;
	trade_action _action;

public:
	constexpr sequence_step(tradable_pair pair, trade_action action)
		: _pair{ std::move(pair) }, _action{ std::move(action) }
	{}

	constexpr const tradable_pair& pair() const noexcept { return _pair; }
	constexpr const trade_action& action() const noexcept { return _action; }

	constexpr bool operator==(const sequence_step& other) const noexcept
	{
		return _action == other._action && _pair == other._pair;
	}
};

class tri_arb_sequence
{
private:
	sequence_step _first;
	sequence_step _middle;
	sequence_step _last;
	std::string _baseCurrency;
	std::string _description;

public:
	constexpr tri_arb_sequence(
		sequence_step first,
		sequence_step middle,
		sequence_step last,
		std::string baseCurrency)
		:
		_first{ std::move(first) },
		_middle{ std::move(middle) },
		_last{ std::move(last) },
		_baseCurrency{ std::move(baseCurrency) },
		_description{ _first.pair().to_standard_string() + "->" + _middle.pair().to_standard_string() + "->" + _last.pair().to_standard_string() }
	{}

	constexpr const sequence_step& first() const noexcept { return _first; }
	constexpr const sequence_step& middle() const noexcept { return _middle; }
	constexpr const sequence_step& last() const noexcept { return _last; }
	constexpr const std::string& base_currency() const noexcept { return _baseCurrency; }
	constexpr const std::string& description() const noexcept { return _description; }
};