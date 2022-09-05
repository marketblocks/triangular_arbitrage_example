#include <iostream>
#include <algorithm>
#include <unordered_set>

#include "triangular_arbitrage.h"
#include "trading/order_book.h"
#include "trading/tradable_pair.h"
#include "exchanges/exchange_helpers.h"
#include "common/utils/mathutils.h"
#include "common/utils/financeutils.h"
#include "logging/logger.h"

namespace
{
	double get_best_price(const mb::order_book_state& orderBook, mb::trade_action action)
	{
		const std::vector<mb::order_book_entry>& entries = action == mb::trade_action::BUY
			? orderBook.asks()
			: orderBook.bids();

		if (entries.empty())
		{
			return 0.0;
		}

		return entries.front().price();
	}

	// Compute all potential pairs and their associated actions for the middle trade of a triangular arbitrage sequence
	std::vector<sequence_step> get_middle_steps(const std::vector<mb::tradable_pair>& allPairs, const sequence_step& firstStep)
	{
		std::vector<sequence_step> steps;

		for (auto& middlePair : allPairs)
		{
			std::string_view gainedAsset{ get_gained_asset(firstStep.pair, firstStep.action) };

			if (!middlePair.contains(gainedAsset) || middlePair == firstStep.pair)
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
		std::string_view baseCurrency = firstStep.action == mb::trade_action::BUY
			? firstStep.pair.price_unit()
			: firstStep.pair.asset();

		std::vector<sequence_step> possibleMiddles{ get_middle_steps(allPairs, firstStep) };

		for (auto& middleStep : possibleMiddles)
		{
			std::string_view middleGainedAsset{ get_gained_asset(middleStep.pair, middleStep.action) };

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
					sequences[firstStep.pair].push_back(sequence);
					sequences[middleStep.pair].push_back(sequence);
					sequences[finalPair].push_back(sequence);

					break;
				}
			}
		}
	}
}

triangular_arbitrage::triangular_arbitrage(tri_arb_config config)
	: 
	_config{ std::move(config) },
	_running{ true }, 
	_started{ false },
	_bookSubConfirmed{ false }, 
	_workerThread{&triangular_arbitrage::consume_book_events, this}
{}

triangular_arbitrage::~triangular_arbitrage()
{
	_running = false;
	_workerThread.join();
}

void triangular_arbitrage::subscribe_to_websocket()
{
	auto stream = _exchange->get_websocket_stream();

	stream->add_order_book_update_handler([this](mb::order_book_update_message message) { book_update_handler(std::move(message)); });
	stream->subscribe(mb::websocket_subscription::create_order_book_sub(_tradingPairs));

	_bookSubConfirmed = false;
}

void triangular_arbitrage::book_update_handler(mb::order_book_update_message message)
{
	_bookMessages.unique_lock()->push(std::move(message));

	if (!_bookSubConfirmed)
	{
		_bookSubConfirmed = true;
		mb::logger::instance().info("Subscription confirmed");
	}
}

void triangular_arbitrage::consume_book_events()
{
	while (_running)
	{
		try
		{
			auto lockedBookMessages = _bookMessages.unique_lock();

			while (!lockedBookMessages->empty())
			{
				mb::order_book_update_message message{ lockedBookMessages->front() };
				lockedBookMessages->pop();

				const std::vector<tri_arb_sequence>& sequences{ _sequences[message.pair()] };
				
				for (auto& sequence : sequences)
				{
					double potentialGain = calculate_potential_gain(sequence);

					if (potentialGain == 0.0)
					{
						continue;
					}

					std::string description{ sequence.first.pair.to_string('/') + "->" + sequence.middle.pair.to_string('/') + "->" + sequence.last.pair.to_string('/') };
					mb::logger::instance().info("Sequence: {0}. Percentage Diff: {1}", description, potentialGain);
				}
			}
		}
		catch (const std::exception& e)
		{
			mb::logger::instance().error(e.what());
		}

		std::this_thread::yield();
	}
}

double triangular_arbitrage::calculate_potential_gain(const tri_arb_sequence& sequence)
{
	auto stream = _exchange->get_websocket_stream();

	double p1{ get_best_price(stream->get_order_book(sequence.first.pair, 1), sequence.first.action) };
	double p2{ get_best_price(stream->get_order_book(sequence.middle.pair, 1), sequence.middle.action) };
	double p3{ get_best_price(stream->get_order_book(sequence.last.pair, 1), sequence.last.action) };

	if (p1 == 0.0 || p2 == 0.0 || p3 == 0.0)
	{
		return 0.0;
	}

	// Helper method for calculating the gained amount from a trade
	constexpr int tradeValue = 1;
	double g1{ calculate_trade_gain(p1, tradeValue, _config.fee(), sequence.first.action)};
	double g2{ calculate_trade_gain(p2, g1, _config.fee(), sequence.middle.action) };
	double g3{ calculate_trade_gain(p3, g2, _config.fee(), sequence.last.action) };

	// Simple percentage diff calculate using (B-A)*100/A
	return mb::calculate_percentage_diff(tradeValue, g3);
}

// Called by runner framework to carry out any initialisation work. 
// Exchanges are all API classes that have been specified in the config file runner.json
void triangular_arbitrage::initialise(std::vector<std::shared_ptr<mb::exchange>> exchanges)
{
	if (exchanges.size() != 1)
	{
		throw mb::mb_exception{ "Strategy only supports one exchange" };
	}

	_exchange = exchanges.front();
	_tradingPairs = _exchange->get_tradable_pairs();

	for (auto& firstPair : _tradingPairs)
	{
		if (firstPair.price_unit() != _config.quote_asset())
		{
			continue;
		}

		// Add sequences that start with a BUY on the first pair
		create_sequences(
			_sequences,
			_tradingPairs,
			sequence_step{ firstPair, mb::trade_action::BUY });

		// Add sequences that start with a SELL on the first pair
		create_sequences(
			_sequences,
			_tradingPairs,
			sequence_step{ firstPair, mb::trade_action::SELL });
	}

	subscribe_to_websocket();
}

// Called by runner framework in continuous loop
void triangular_arbitrage::run_iteration()
{
	if (!_started && !_bookSubConfirmed)
	{
		mb::logger::instance().info("Waiting for subscription confirmation...");
		_started = true;
	}

	auto stream = _exchange->get_websocket_stream();

	if (stream->connection_status() != mb::ws_connection_status::OPEN)
	{
		stream->reset();
		subscribe_to_websocket();
	}
}