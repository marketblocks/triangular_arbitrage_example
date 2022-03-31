﻿#include "runner/runner.h"
#include "logging/logger.h"
#include "triangular_arbitrage.h"

int main()
{
	try
	{
		// Create runner for strategy
		mb::runner<triangular_arbitrage> runner{};

		// Start initialisation phase
		runner.initialise();

		// Begin running strategy
		runner.run();
	}
	catch (const std::exception& e)
	{
		mb::logger::instance().critical(e.what());
	}

	return 0;
}
