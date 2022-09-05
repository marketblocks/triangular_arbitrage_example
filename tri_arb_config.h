#pragma once

#include "common/json/json.h"

class tri_arb_config
{
private:
	std::string _quoteAsset;
	double _fee;

public:
	tri_arb_config();
	tri_arb_config(std::string quoteAsset, double fee);

	static std::string name() noexcept { return "tri_arb"; }

	const std::string& quote_asset() const noexcept { return _quoteAsset; }
	double fee() const noexcept { return _fee; }
};

template<>
tri_arb_config mb::from_json(const mb::json_document& json);

template<>
void mb::to_json(const tri_arb_config& config, mb::json_writer& writer);