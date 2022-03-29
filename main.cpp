#include <spdlog/spdlog.h>

#include "runner/runner.h"
#include "logging/logger.h"
#include "strategies/tri_arb.h"

int main()
{
	cb::runner<tri_arb_strategy> runner{};

	try
	{
		runner.initialise();
		runner.run();
	}
	catch (const std::exception& e)
	{
		cb::logger::instance().critical(e.what());
	}

	spdlog::shutdown();
	return 0;
}
