#include <iostream>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

template <size_t N>
memory_ptr get(slab_hash_table_header& header, slab_manager& alloc,
    const data_chunk& key_data)
{
    typedef byte_array<N> hash_type;
    slab_hash_table<hash_type> ht(header, alloc);
    hash_type key;
    BITCOIN_ASSERT(key.size() == key_data.size());
    std::copy(key_data.begin(), key_data.end(), key.begin());
    return ht.find(key);
}

int main(int argc, char** argv)
{
    if (argc != 5 && argc != 6)
    {
        std::cerr
            << "Usage: read_htdb_slab_value FILENAME KEY VALUE_SIZE BUCKETS "
            << "[OFFSET]" << std::endl;
        return 0;
    }

    const std::string filename = argv[1];

    data_chunk key_data;
    if (!decode_base16(key_data, argv[2]))
    {
        std::cerr << "key data is not valid" << std::endl;
        return -1;
    }

    const auto value_size = lexical_cast<size_t>(argv[3]);
    const auto buckets = lexical_cast<array_index>(argv[4]);

    file_offset offset = 0;
    if (argc == 6)
        offset = lexical_cast<file_offset>(argv[5]);

    memory_map file(filename);
    auto result = file.open();
    BITCOIN_ASSERT(result);

    slab_hash_table_header header(file, buckets);
    result = header.start();
    BITCOIN_ASSERT(result);

    slab_manager manager(file, offset + 4 + 8 * header.size());
    result = manager.start();
    BITCOIN_ASSERT(result);

    memory_ptr slab = nullptr;
    if (key_data.size() == 32)
    {
        slab = get<32>(header, manager, key_data);
    }
    else if (key_data.size() == 4)
    {
        slab = get<4>(header, manager, key_data);
    }
    else
    {
        std::cerr << "read_htdb_slab_value: unsupported KEY size, use 4 or 32."
            << std::endl;
        return -1;
    }

    const auto memory = REMAP_ADDRESS(slab);
    data_chunk data(memory, memory + value_size);
    std::cout << encode_base16(data) << std::endl;
    return 0;
}

