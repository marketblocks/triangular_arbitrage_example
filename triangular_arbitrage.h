#pragma once

#include <vector>

#include "sequences.h"
#include "exchanges/exchange.h"

class triangular_arbitrage
{
private:
	std::vector<std::shared_ptr<mb::exchange>> _exchanges;
	mb::unordered_string_map<tri_arb_exchange_data> _exchangeData;

public:
	triangular_arbitrage() {}

	void initialise(std::vector<std::shared_ptr<mb::exchange>> exchanges);
	void run_iteration();
};