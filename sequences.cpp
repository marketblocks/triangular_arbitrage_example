#include "sequences.h"
#include "common/utils/containerutils.h"

tri_arb_exchange_data::tri_arb_exchange_data(std::unordered_map<mb::tradable_pair, std::vector<tri_arb_sequence>> sequences)
	: _sequences{ std::move(sequences) }
{}

std::vector<mb::tradable_pair> tri_arb_exchange_data::get_all_sequence_pairs() const
{
	return to_vector<mb::tradable_pair>(_sequences, [](const auto& item)
		{
			return item.first;
		});
}

const std::vector<tri_arb_sequence>& tri_arb_exchange_data::get_sequences(const mb::tradable_pair& pair) const
{
	auto it = _sequences.find(pair);

	if (it == _sequences.end())
	{
		return {};
	}

	return it->second;
}