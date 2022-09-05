#include "tri_arb_config.h"

namespace
{
	namespace json_property_names
	{
		static constexpr std::string_view QUOTE_ASSET = "quoteAsset";
		static constexpr std::string_view FEE = "fee";
	}
}

tri_arb_config::tri_arb_config()
	: _quoteAsset{ "USDT" }, _fee{ 0.1 }
{}

tri_arb_config::tri_arb_config(std::string quoteAsset, double fee)
	: _quoteAsset{ std::move(quoteAsset) }, _fee{ fee }
{}

template<>
tri_arb_config mb::from_json(const mb::json_document& json)
{
	return tri_arb_config
	{
		json.get<std::string>(json_property_names::QUOTE_ASSET),
		json.get<double>(json_property_names::FEE)
	};
}

template<>
void mb::to_json(const tri_arb_config& config, mb::json_writer& writer)
{
	writer.add(json_property_names::QUOTE_ASSET, config.quote_asset());
	writer.add(json_property_names::FEE, config.fee());
}