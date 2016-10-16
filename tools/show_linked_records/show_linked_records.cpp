/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <cstdint>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

struct chain_item
{
    array_index index;
    data_chunk data;
};

int main(int argc, char** argv)
{
    typedef std::vector<chain_item> chain_type;
    typedef std::vector<chain_type> chain_list;

    if (argc != 3 && argc != 4)
    {
        std::cerr << "Usage: show_records FILENAME RECORD_SIZE [OFFSET]"
            << std::endl;
        return 0;
    }

    const std::string filename = argv[1];
    const auto record_size = record_list_offset + lexical_cast<size_t>(argv[2]);

    file_offset offset = 0;
    if (argc == 4)
        offset = lexical_cast<file_offset>(argv[3]);

    memory_map file(filename);
    record_manager manager(file, offset, record_size);
    const auto result = manager.start();
    BITCOIN_ASSERT(result);

    record_list lrs(manager);
    chain_list chains;

    for (array_index index = 0; index < manager.count(); ++index)
    {
        BITCOIN_ASSERT(record_size >= 4);
        const auto memory = manager.get(index);
        const auto buffer = REMAP_ADDRESS(memory);
        const auto previous_index = from_little_endian_unsafe<uint32_t>(buffer);
        const data_chunk data(buffer + 4, buffer + record_size);
        const chain_item new_item{ index, data };

        if (previous_index == record_list::empty)
        {
            // Create new chain
            chain_type chain{ { new_item } };
            chains.push_back(chain);
            continue;
        }

        auto found = false;

        // Iterate all chains, looking for match.
        for (auto& chain: chains)
        {
            for (auto& item: chain)
            {
                if (item.index == previous_index)
                {
                    chain.push_back(new_item);
                    found = true;
                    break;
                }
            }

            if (found)
                break;
        }

        BITCOIN_ASSERT_MSG(found, "Internal error or bad file.");
    }

    // Chains are complete, now display them.
    for (size_t chain_index = 0; chain_index < chains.size(); ++chain_index)
    {
        std::cout << chain_index << ":" << std::endl;
        const auto& chain = chains[chain_index];

        for (size_t item_index = 0; item_index < chain.size(); ++item_index)
        {
            auto& item = chain[item_index];
            std::cout << "  " << item_index << " (@" << item.index
                << "): " << encode_base16(item.data) << std::endl;
        }
    }
}

