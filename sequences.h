#pragma once

#include "trading/tradable_pair.h"
#include "trading/trading_constants.h"

struct sequence_step
{
	mb::tradable_pair pair;
	mb::trade_action action;
};

struct tri_arb_sequence
{
	sequence_step first;
	sequence_step middle;
	sequence_step last;
	std::string baseCurrency;
};