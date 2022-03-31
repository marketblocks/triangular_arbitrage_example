#pragma once

#include "exchanges/exchange.h"
#include "tri_arb_sequence.h"

using namespace cb;

class tri_arb_spec
{
private:
	std::shared_ptr<exchange> _exchange;
	std::unordered_map<tradable_pair, std::vector<tri_arb_sequence>> _sequences;

public:
	tri_arb_spec(
		std::shared_ptr<exchange> exchange,
		std::unordered_map<tradable_pair, std::vector<tri_arb_sequence>> sequences);

	std::shared_ptr<exchange> exchange() const noexcept { return _exchange; }
	set_queue<tradable_pair>& order_book_message_queue() const noexcept { return _exchange->get_websocket_stream().get_order_book_message_queue(); }

	std::vector<tradable_pair> get_all_tradable_pairs() const;
	const std::vector<tri_arb_sequence>& get_sequences(const tradable_pair& pair) const;
};