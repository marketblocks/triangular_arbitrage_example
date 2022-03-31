#pragma once

#include <vector>

#include "tri_arb_spec.h"
#include "runner/initialise/strategy_initialiser.h"

class triangular_arbitrage
{
private:
	std::vector<tri_arb_spec> _specs;

public:
	triangular_arbitrage() {}

	void initialise(const mb::strategy_initialiser& initialiser);
	void run_iteration();
};