#include "runner/runner.h"
#include "logging/logger.h"
#include "strategies/tri_arb.h"

int main()
{
	try
	{
		cb::runner<tri_arb_strategy> runner{};
		runner.initialise();
		runner.run();
	}
	catch (const std::exception& e)
	{
		cb::logger::instance().critical(e.what());
	}

	return 0;
}
