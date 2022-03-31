#include "tri_arb_spec.h"
#include "common/utils/containerutils.h"

tri_arb_spec::tri_arb_spec(
	std::shared_ptr<mb::exchange> exchange,
	std::unordered_map<mb::tradable_pair, std::vector<tri_arb_sequence>> sequences)
	:
	_exchange{ exchange },
	_sequences{ std::move(sequences) }
{}

std::vector<mb::tradable_pair> tri_arb_spec::get_all_tradable_pairs() const
{
	return to_vector<mb::tradable_pair>(_sequences, [](const auto& item)
		{
			return item.first;
		});
}

const std::vector<tri_arb_sequence>& tri_arb_spec::get_sequences(const mb::tradable_pair& pair) const
{
	auto it = _sequences.find(pair);

	if (it == _sequences.end())
	{
		throw mb::cb_exception{ "No sequences for pair" };
	}

	return it->second;
}