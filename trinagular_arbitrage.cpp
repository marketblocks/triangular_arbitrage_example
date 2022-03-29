#include <iostream>
#include <algorithm>
#include <unordered_set>

#include "triangular_arbitrage.h"
#include "trading/order_book.h"
#include "trading/trade_description.h"
#include "exchanges/exchange_helpers.h"
#include "common/utils/mathutils.h"
#include "common/utils/financeutils.h"
#include "logging/logger.h"

using namespace cb;

tri_arb_spec::tri_arb_spec(
	std::shared_ptr<cb::exchange> exchange, 
	std::unordered_map<tradable_pair, std::vector<tri_arb_sequence>> sequences)
	: 
	_exchange{ exchange }, 
	_sequences{ std::move(sequences) }
{}

std::vector<tradable_pair> tri_arb_spec::get_all_tradable_pairs() const
{
	return to_vector<tradable_pair>(_sequences, [](const auto& item)
		{ 
			return item.first;
		});
}

const std::vector<tri_arb_sequence>& tri_arb_spec::get_sequences(const tradable_pair& pair) const
{
	auto it = _sequences.find(pair);

	if (it == _sequences.end())
	{
		throw cb_exception{ "No sequences for pair" };
	}

	return it->second;
}

namespace
{
	struct SequenceTrades
	{
		cb::trade_description first;
		cb::trade_description middle;
		cb::trade_description last;
	};

	struct SequenceTradeStep
	{
		cb::trade_description description;
		double gainValue;
	};

	struct SequenceGains
	{
		double g1;
		double g2;
		double g3;
	};

	cb::trade_description create_trade(const sequence_step& sequenceStep, order_book_entry bestEntry, double g1, double g0)
	{
		double volume = sequenceStep.action() == cb::trade_action::BUY ? g1 : g0;
		double assetPrice = bestEntry.price();

		return cb::trade_description(cb::order_type::LIMIT, sequenceStep.pair(), sequenceStep.action(), assetPrice, volume);
	}

	SequenceTrades create_sequence_trades(const tri_arb_sequence& sequence, const SequenceGains& gains, const std::unordered_map<cb::tradable_pair, cb::order_book_entry>& bestOrderBookEntries, double initialTradeValue)
	{
		cb::trade_description firstTrade = create_trade(sequence.first(), bestOrderBookEntries.at(sequence.first().pair()), gains.g1, initialTradeValue);
		cb::trade_description middleTrade = create_trade(sequence.middle(), bestOrderBookEntries.at(sequence.first().pair()), gains.g2, gains.g1);
		cb::trade_description lastTrade = create_trade(sequence.last(), bestOrderBookEntries.at(sequence.first().pair()), gains.g3, gains.g2);

		return SequenceTrades{ std::move(firstTrade), std::move(middleTrade), std::move(lastTrade) };
	}

	SequenceGains calculate_sequence_gains(const tri_arb_sequence& sequence, const std::unordered_map<cb::tradable_pair, cb::order_book_entry>& bestOrderBookEntries, const std::unordered_map<cb::tradable_pair, double>& fees, double initialTradeValue)
	{
		const sequence_step& firstStep = sequence.first();
		const sequence_step& middleStep = sequence.middle();
		const sequence_step& lastStep = sequence.last();

		double firstFee = fees.at(firstStep.pair());
		double middleFee = fees.at(middleStep.pair());
		double lastFee = fees.at(lastStep.pair());

		double g1 = calculate_trade_gain(bestOrderBookEntries.at(firstStep.pair()).price(), initialTradeValue, firstFee, firstStep.action());
		double g2 = calculate_trade_gain(bestOrderBookEntries.at(middleStep.pair()).price(), g1, middleFee, middleStep.action());
		double g3 = calculate_trade_gain(bestOrderBookEntries.at(lastStep.pair()).price(), g2, lastFee, lastStep.action());

		return SequenceGains{ g1, g2, g3 };
	}

	order_book_entry get_best_entry(std::shared_ptr<exchange> exchange, sequence_step sequenceStep)
	{
		return select_best_entry(
			exchange->get_websocket_stream().get_order_book(sequenceStep.pair(), 1), 
			sequenceStep.action());
	}
}

void tri_arb_strategy::initialise(const cb::strategy_initialiser& initaliser)
{
	for (auto exchange : initaliser.exchanges())
	{
		std::vector<std::string> baseCurrencies;
		/*{
			"GBP"
		};*/
		cb::unordered_string_map<double> balances{ exchange->get_balances() };

		for (auto& [asset, balance] : balances)
		{
			if (balance > 0)
			{
				baseCurrencies.push_back(asset);
			}
		}

		tri_arb_spec& spec = _specs.emplace_back(create_exchange_spec(exchange, baseCurrencies));

		cb::websocket_stream& websocketStream = exchange->get_websocket_stream();
		websocketStream.connect();
		websocketStream.subscribe_order_book(spec.get_all_tradable_pairs());
	}
}

void tri_arb_strategy::run_iteration()
{
	for (auto& spec : _specs)
	{
		auto& messageQueue = spec.exchange()->get_websocket_stream().get_order_book_message_queue();

		while (!messageQueue.empty())
		{
			const std::vector<tri_arb_sequence>& sequences{ spec.get_sequences(messageQueue.pop()) };
			std::shared_ptr<exchange> exchange{ spec.exchange() };

			for (auto& sequence : sequences)
			{
				try
				{
					double tradeValue = 0.05 * get_balance(*exchange, sequence.first().pair().price_unit());

					std::unordered_map<tradable_pair, order_book_entry> orderBooks;
					orderBooks.emplace(sequence.first().pair(), get_best_entry(exchange, sequence.first()));
					orderBooks.emplace(sequence.middle().pair(), get_best_entry(exchange, sequence.middle()));
					orderBooks.emplace(sequence.last().pair(), get_best_entry(exchange, sequence.last()));

					const std::unordered_map<cb::tradable_pair, double> fees
					{
						{ sequence.first().pair(), 0.26},
						{ sequence.middle().pair(), 0.26 },
						{ sequence.last().pair(), 0.26 }
					};

					SequenceGains gains = calculate_sequence_gains(sequence, orderBooks, fees, tradeValue);

					if (gains.g3 > tradeValue)
					{
						/*SequenceTrades trades = create_sequence_trades(sequence, gains, orderBooks, tradeValue);

						exchange->add_order(trades.first);
						exchange->add_order(trades.middle);
						exchange->add_order(trades.last);*/

						cb::logger::instance().info("FOUND TRADE!!! Sequence: {0}. Percentage Diff : {1}", sequence.description(), cb::calculate_percentage_diff(tradeValue, gains.g3));

						//log_trade(sequence, trades, gains, tradeValue, get_balance(exchange, _options.fiat_currency()));
					}
					else
					{
						cb::logger::instance().info("Sequence: {0}. Percentage Diff: {1}", sequence.description(), cb::calculate_percentage_diff(tradeValue, gains.g3));
					}
				}
				catch (const cb::cb_exception& e)
				{
						
					//cb::logger::instance().error("Error occurred during sequence {0}: {1}", to_string(sequence), e.what());
				}
			}
		}

		//cb::logger::instance().info("Queue empty");
	}

	//for (auto& spec : _specs)
	//{
	//	cb::exchange& exchange = spec.exchange();
	//	double tradeValue = _options.max_trade_percent() * get_balance(exchange, _options.fiat_currency());

	//	for (auto& sequence : spec.sequences())
	//	{
	//		try
	//		{
	//			const std::unordered_map<cb::tradable_pair, cb::order_book_level> prices = get_best_order_book_prices(exchange.get_websocket_stream(), sequence.pairs());
	//			const std::unordered_map<cb::tradable_pair, double> fees
	//			{
	//				{ sequence.pairs()[0], 0.26 },
	//				{ sequence.pairs()[1], 0.26 },
	//				{ sequence.pairs()[2], 0.26 }
	//			};
	//			//exchange.get_fees(sequence.pairs());

	//			SequenceGains gains = calculate_sequence_gains(sequence, prices, fees, tradeValue);

	//			if (gains.g3 > tradeValue)
	//			{
	//				SequenceTrades trades = create_sequence_trades(sequence, gains, prices, tradeValue);

	//				exchange.add_order(trades.first);
	//				exchange.add_order(trades.middle);
	//				exchange.add_order(trades.last);

	//				//log_trade(sequence, trades, gains, tradeValue, get_balance(exchange, _options.fiat_currency()));
	//			}
	//			else
	//			{
	//				cb::logger::instance().info("Sequence: {0}. Percentage Diff: {1}", to_string(sequence), cb::calculate_percentage_diff(tradeValue, gains.g3));
	//			}
	//		}
	//		catch (const cb::cb_exception& e)
	//		{
	//			
	//			cb::logger::instance().error("Error occurred during sequence {0}: {1}", to_string(sequence), e.what());
	//		}
	//	}
	//}
}