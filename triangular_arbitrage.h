#pragma once

#include <vector>

#include "tri_arb_spec.h"
#include "runner/initialise/strategy_initialiser.h"

using namespace cb;

class triangular_arbitrage
{
private:
	std::vector<tri_arb_spec> _specs;

public:
	triangular_arbitrage() {}

	void initialise(const cb::strategy_initialiser& initialiser);
	void run_iteration();
};