#pragma once

#include "exchanges/exchange.h"
#include "tri_arb_sequence.h"

class tri_arb_spec
{
private:
	std::shared_ptr<cb::exchange> _exchange;
	std::unordered_map<cb::tradable_pair, std::vector<tri_arb_sequence>> _sequences;

public:
	tri_arb_spec(
		std::shared_ptr<cb::exchange> exchange,
		std::unordered_map<cb::tradable_pair, std::vector<tri_arb_sequence>> sequences);

	std::shared_ptr<cb::exchange> exchange() const noexcept { return _exchange; }
	cb::set_queue<cb::tradable_pair>& order_book_message_queue() const noexcept { return _exchange->get_websocket_stream().get_order_book_message_queue(); }

	std::vector<cb::tradable_pair> get_all_tradable_pairs() const;
	const std::vector<tri_arb_sequence>& get_sequences(const cb::tradable_pair& pair) const;
};