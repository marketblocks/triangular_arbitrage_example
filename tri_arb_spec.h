#pragma once

#include "exchanges/exchange.h"
#include "tri_arb_sequence.h"

class tri_arb_spec
{
private:
	std::shared_ptr<mb::exchange> _exchange;
	std::unordered_map<mb::tradable_pair, std::vector<tri_arb_sequence>> _sequences;

public:
	tri_arb_spec(
		std::shared_ptr<mb::exchange> exchange,
		std::unordered_map<mb::tradable_pair, std::vector<tri_arb_sequence>> sequences);

	std::shared_ptr<mb::exchange> exchange() const noexcept { return _exchange; }
	mb::set_queue<mb::tradable_pair>& order_book_message_queue() const noexcept { return _exchange->get_websocket_stream().get_order_book_message_queue(); }

	std::vector<mb::tradable_pair> get_all_tradable_pairs() const;
	const std::vector<tri_arb_sequence>& get_sequences(const mb::tradable_pair& pair) const;
};