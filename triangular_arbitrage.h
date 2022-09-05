#pragma once

#include <vector>

#include "tri_arb_config.h"
#include "sequences.h"
#include "exchanges/exchange.h"
#include "testing/reporting/test_report.h"

class triangular_arbitrage
{
private:
	tri_arb_config _config;
	std::shared_ptr<mb::exchange> _exchange;
	std::unordered_map<mb::tradable_pair, std::vector<tri_arb_sequence>> _sequences;
	std::vector<mb::tradable_pair> _tradingPairs;
	std::thread _workerThread;
	bool _running;
	bool _started;
	bool _bookSubConfirmed;

	mb::concurrent_wrapper<std::queue<mb::order_book_update_message>> _bookMessages;

	void subscribe_to_websocket();
	void book_update_handler(mb::order_book_update_message message);
	void consume_book_events();
	double calculate_potential_gain(const tri_arb_sequence& sequence);

public:
	triangular_arbitrage(tri_arb_config config);
	~triangular_arbitrage();

	void initialise(std::vector<std::shared_ptr<mb::exchange>> exchanges);
	void run_iteration();
	mb::report_result_list get_test_results() { return {}; }
};