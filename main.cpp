#include "runner/runner.h"
#include "logging/logger.h"
#include "triangular_arbitrage.h"

int main()
{
	// Create runner for strategy
	mb::runner<triangular_arbitrage> runner = mb::create_runner<triangular_arbitrage>();

	// Start initialisation phase
	runner.initialise();

	// Begin running strategy
	runner.run();

	return 0;
}
