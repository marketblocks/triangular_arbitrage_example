#include "runner/runner.h"
#include "logging/logger.h"
#include "triangular_arbitrage.h"

int main()
{
	// Create runner for strategy
	mb::runner<triangular_arbitrage> runner = mb::create_runner<triangular_arbitrage>();

	// Start initialisation phase
	tri_arb_config config{ runner.load_custom_config<tri_arb_config>() };
	runner.initialise(std::move(config));

	// Begin running strategy
	runner.run();

	return 0;
}
