#include <iostream>
#include <algorithm>
#include <unordered_set>

#include "triangular_arbitrage.h"
#include "trading/order_book.h"
#include "trading/trade_description.h"
#include "trading/tradable_pair.h"
#include "exchanges/exchange_helpers.h"
#include "common/utils/mathutils.h"
#include "common/utils/financeutils.h"
#include "logging/logger.h"

namespace
{
	struct sequence_prices
	{
		mb::order_book_entry firstEntry;
		double firstFee;

		mb::order_book_entry middleEntry;
		double middleFee;

		mb::order_book_entry lastEntry;
		double lastFee;
	};

	// Helper method for creating a fee-adjusted trade description of a specified value
	mb::trade_description create_trade(const sequence_step& sequenceStep, double assetPrice, double tradeValue, double fee)
	{
		double volume = mb::calculate_volume(assetPrice, tradeValue);

		if (sequenceStep.action() == mb::trade_action::BUY)
		{
			// If buying, adjust volume down
			volume -= mb::calculate_fee(tradeValue, fee);
		}
		else
		{
			// If selling, adjust volume up
			volume += mb::calculate_fee(volume, fee);
		}

		return mb::trade_description(mb::order_type::LIMIT, sequenceStep.pair(), sequenceStep.action(), assetPrice, volume);
	}

	double calculate_potential_gain(const tri_arb_sequence& sequence, const sequence_prices& prices)
	{
		constexpr int tradeValue = 1;

		// Helper method for calculating the gained amount from a trade
		double g1 = calculate_trade_gain(prices.firstEntry.price(), tradeValue, prices.firstFee, sequence.first().action());
		double g2 = calculate_trade_gain(prices.middleEntry.price(), g1, prices.middleFee, sequence.middle().action());
		double g3 = calculate_trade_gain(prices.lastEntry.price(), g2, prices.lastFee, sequence.last().action());

		// Simple percentage diff calculate using (B-A)/A
		return mb::calculate_percentage_diff(tradeValue, g3);
	}

	// Helper method for selecting the best order book entry on a particular side
	mb::order_book_entry get_best_entry(std::shared_ptr<mb::exchange> exchange, sequence_step sequenceStep)
	{
		return select_best_entry(
			// get_order_book returns a snapshot of the current order book for a given pair at a specified depth
			exchange->get_websocket_stream().lock()->get_order_book(sequenceStep.pair(), 1),
			sequenceStep.action());
	}

	// Helper method to check if all pairs in sequence are subscribed to
	bool is_subscribed_to_all(std::shared_ptr<mb::websocket_stream> websocketStream, const tri_arb_sequence& sequence)
	{
		return 
			websocketStream->is_order_book_subscribed(sequence.first().pair()) &&
			websocketStream->is_order_book_subscribed(sequence.middle().pair()) &&
			websocketStream->is_order_book_subscribed(sequence.last().pair());
	}

	// Compute all potential pairs and their associated actions for the middle trade of a triangular arbitrage sequence
	std::vector<sequence_step> get_middle_steps(const std::vector<mb::tradable_pair>& allPairs, const sequence_step& firstStep)
	{
		std::vector<sequence_step> steps;

		for (auto& middlePair : allPairs)
		{
			std::string_view gainedAsset{ get_gained_asset(firstStep.pair(), firstStep.action()) };

			if (!middlePair.contains(gainedAsset) || middlePair == firstStep.pair())
			{
				continue;
			}

			mb::trade_action action = gainedAsset == middlePair.asset()
				? mb::trade_action::SELL
				: mb::trade_action::BUY;

			steps.emplace_back(middlePair, action);
		}

		return steps;
	}

	// Compute all possible triangular sequence for a given starting pair and action (buy/sell)
	void create_sequences(
		std::unordered_map<mb::tradable_pair, std::vector<tri_arb_sequence>>& sequences,
		const std::vector<mb::tradable_pair>& allPairs,
		const sequence_step& firstStep)
	{
		std::string_view baseCurrency = firstStep.action() == mb::trade_action::BUY
			? firstStep.pair().price_unit()
			: firstStep.pair().asset();

		std::vector<sequence_step> possibleMiddles{ get_middle_steps(allPairs, firstStep) };

		for (auto& middleStep : possibleMiddles)
		{
			std::string_view middleGainedAsset{ get_gained_asset(middleStep.pair(), middleStep.action()) };

			for (auto& finalPair : allPairs)
			{
				if (finalPair.contains(baseCurrency) && finalPair.contains(middleGainedAsset))
				{
					mb::trade_action finalAction = finalPair.price_unit() == middleGainedAsset
						? mb::trade_action::BUY
						: mb::trade_action::SELL;

					tri_arb_sequence sequence
					{
						firstStep, middleStep, sequence_step{ finalPair, finalAction }, std::string{ baseCurrency }
					};

					// Cache sequence for each associated pair
					sequences[firstStep.pair()].push_back(sequence);
					sequences[middleStep.pair()].push_back(sequence);
					sequences[finalPair].push_back(sequence);

					break;
				}
			}
		}
	}

	tri_arb_exchange_data get_exchange_data(std::shared_ptr<mb::exchange> exchange)
	{
		// Exchange REST endpoint for getting all pairs that can be traded on the exchange
		std::vector<mb::tradable_pair> tradablePairs = exchange->get_tradable_pairs();
		std::unordered_map<mb::tradable_pair, std::vector<tri_arb_sequence>> sequences;

		for (auto& firstPair : tradablePairs)
		{
			// Add sequences that start with a BUY on the first pair
			create_sequences(
				sequences,
				tradablePairs,
				sequence_step{ firstPair, mb::trade_action::BUY });

			// Add sequences that start with a SELL on the first pair
			create_sequences(
				sequences,
				tradablePairs,
				sequence_step{ firstPair, mb::trade_action::SELL });
			
		}

		return tri_arb_exchange_data{ std::move(sequences) };
	}
}

// Called by runner framework to carry out any initialisation work. 
// Exchanges are all API classes that have been specified in the config file runner.json
void triangular_arbitrage::initialise(std::vector<std::shared_ptr<mb::exchange>> exchanges)
{
	_exchangeData.reserve(exchanges.size());

	for (auto exchange : exchanges)
	{
		// Compute all possible triangular sequences for given exchange
		tri_arb_exchange_data exchangeData = get_exchange_data(exchange);

		// Subscribe to exchange's order book websocket feed for all trading pairs identified as part of a valid sequence
		std::shared_ptr<mb::websocket_stream> websocketStream = exchange->get_websocket_stream().lock();

		if (websocketStream)
		{
			websocketStream->connect();
			websocketStream->subscribe_order_book(exchangeData.get_all_sequence_pairs());
		}

		_exchangeData.emplace(exchange->id(), std::move(exchangeData));
	}

	_exchanges = std::move(exchanges);
}

// Called by runner framework in continuous loop
void triangular_arbitrage::run_iteration()
{
	for (auto exchange : _exchanges)
	{
		const tri_arb_exchange_data& exchangeData = _exchangeData.find(exchange->id())->second;
		auto websocketStream = exchange->get_websocket_stream().lock();

		if (!websocketStream)
		{
			continue;
		}

		// order_book_message_queue is a FIFO queue containing the tradable pairs with order book changes
		while (!websocketStream->get_order_book_message_queue().empty())
		{
			// Retrieve the pre-computed sequences relating to the particular trading pair
			const std::vector<tri_arb_sequence>& sequences{ exchangeData.get_sequences(websocketStream->get_order_book_message_queue().pop()) };

			for (auto& sequence : sequences)
			{
				if (!is_subscribed_to_all(websocketStream, sequence))
				{
					continue;
				}

				sequence_prices prices
				{
					get_best_entry(exchange, sequence.first()), // First pair order book entry
					
					// Authenticated REST endpoint for getting the trading fee usually based on traded volume
					exchange->get_fee(sequence.first().pair()), // First pair trading fee
					get_best_entry(exchange, sequence.middle()), // Middle pair
					exchange->get_fee(sequence.middle().pair()),
					get_best_entry(exchange, sequence.last()), // Final pair
					exchange->get_fee(sequence.last().pair())
				};

				double potentialGain = calculate_potential_gain(sequence, prices);
				mb::logger::instance().info("Sequence: {0}. Percentage Diff: {1}", sequence.description(), potentialGain);

				if (potentialGain > 0)
				{
					// Limit trade value to 5% of the available balance of the base currency of this sequence
					constexpr int tradeValuePercentage = 0.05;
					double tradeValue = tradeValuePercentage * get_balance(exchange, sequence.base_currency());

					// Assuming sufficient volume available at price points, execute trades
					// Exchange REST endpoint for adding a new order
					exchange->add_order(create_trade(sequence.first(), prices.firstEntry.price(), tradeValue, prices.firstFee));
					exchange->add_order(create_trade(sequence.middle(), prices.middleEntry.price(), tradeValue, prices.middleFee));
					exchange->add_order(create_trade(sequence.last(), prices.lastEntry.price(), tradeValue, prices.lastFee));
				}
			}
		}
	}
}