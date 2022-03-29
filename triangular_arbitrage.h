#pragma once

#include <memory>
#include <vector>
#include <iostream>

#include "trading/tradable_pair.h"
#include "trading/trading_constants.h"
#include "exchanges/exchange.h"
#include "runner/initialise/strategy_initialiser.h"
#include "common/utils/containerutils.h"
#include "common/types/set_queue.h"

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
	std::string _description;

public:
	constexpr tri_arb_sequence(
		sequence_step first,
		sequence_step middle, 
		sequence_step last)
		:
		_first{ std::move(first) }, 
		_middle{ std::move(middle) },
		_last{ std::move(last) },
		_description{ _first.pair().to_standard_string() + "->" + _middle.pair().to_standard_string() + "->" + _last.pair().to_standard_string() }
	{}

	constexpr const sequence_step& first() const noexcept { return _first; }
	constexpr const sequence_step& middle() const noexcept { return _middle; }
	constexpr const sequence_step& last() const noexcept { return _last; }
	constexpr const std::string& description() const noexcept { return _description; }
};

class tri_arb_spec
{
private:
	std::shared_ptr<exchange> _exchange;
	std::unordered_map<tradable_pair, std::vector<tri_arb_sequence>> _sequences;

public:
	tri_arb_spec(
		std::shared_ptr<exchange> exchange, 
		std::unordered_map<tradable_pair, std::vector<tri_arb_sequence>> sequences);

	std::shared_ptr<exchange> exchange() const { return _exchange; }

	std::vector<tradable_pair> get_all_tradable_pairs() const;
	const std::vector<tri_arb_sequence>& get_sequences(const tradable_pair& pair) const;
};

class tri_arb_strategy
{
private:
	std::vector<tri_arb_spec> _specs;

public:
	tri_arb_strategy() {}

	void initialise(const cb::strategy_initialiser& initialiser);
	void run_iteration();
};

template<typename BaseCurrencyList>
tri_arb_spec create_exchange_spec(std::shared_ptr<cb::exchange> exchange, const BaseCurrencyList& baseCurrencies)
{
	std::vector<cb::tradable_pair> tradablePairs = exchange->get_tradable_pairs();
	std::unordered_map<tradable_pair, std::vector<tri_arb_sequence>> sequences;

	std::vector<cb::tradable_pair> firstTradables = cb::copy_where<std::vector<cb::tradable_pair>>(
		tradablePairs,
		[&baseCurrencies](const cb::tradable_pair& pair) 
		{ 
			for (auto currency : baseCurrencies)
			{
				if (pair.contains(currency))
					return true;
			}

			return false;
		});

	std::vector<cb::tradable_pair> nonFirstTradables = cb::copy_where<std::vector<cb::tradable_pair>>(
		tradablePairs,
		[&firstTradables](const cb::tradable_pair& pair) { return !cb::contains(firstTradables, pair); });

	for (auto& firstPair : firstTradables)
	{
		cb::trade_action firstAction;
		std::string firstGainedAsset;
		std::string baseCurrency;

		if (cb::contains(baseCurrencies, firstPair.price_unit()))
		{
			firstAction = cb::trade_action::BUY;
			firstGainedAsset = firstPair.asset();
			baseCurrency = firstPair.price_unit();
		}
		else
		{
			firstAction = cb::trade_action::SELL;
			firstGainedAsset = firstPair.price_unit();
			baseCurrency = firstPair.asset();
		}

		std::vector<cb::tradable_pair> possibleMiddles = cb::copy_where<std::vector<cb::tradable_pair>>(
			nonFirstTradables,
			[&firstGainedAsset](const cb::tradable_pair& pair) { return pair.contains(firstGainedAsset); });

		for (auto& middlePair : possibleMiddles)
		{
			cb::trade_action middleAction;
			std::string middleGainedAsset;

			if (middlePair.price_unit() == firstGainedAsset)
			{
				middleAction = cb::trade_action::BUY;
				middleGainedAsset = middlePair.asset();
			}
			else
			{
				middleAction = cb::trade_action::SELL;
				middleGainedAsset = middlePair.price_unit();
			}

			std::vector<cb::tradable_pair> finals = cb::copy_where<std::vector<cb::tradable_pair>>(
				firstTradables,
				[&middleGainedAsset, &baseCurrency](const cb::tradable_pair& pair) { return pair.contains(middleGainedAsset) && pair.contains(baseCurrency); });

			for (auto& finalPair : finals)
			{
				cb::trade_action finalAction = finalPair.price_unit() == middleGainedAsset
					? cb::trade_action::BUY
					: cb::trade_action::SELL;

				tri_arb_sequence sequence
				{
					sequence_step{ firstPair, firstAction },
					sequence_step{ middlePair, middleAction },
					sequence_step{ finalPair, finalAction }
				};

				sequences[firstPair].push_back(sequence);
				sequences[middlePair].push_back(sequence);
				sequences[finalPair].push_back(sequence);

				break;
			}
		}
	}

	return tri_arb_spec{ exchange, std::move(sequences) };
}