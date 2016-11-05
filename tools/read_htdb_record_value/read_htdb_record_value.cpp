#include <cstdint>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

static const size_t buckets = 1000;

template <size_t N>
memory_ptr get(record_hash_table_header& header, record_manager& alloc,
    const data_chunk& key_data)
{
    typedef byte_array<N> hash_type;
    record_hash_table<hash_type> ht(header, alloc);
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
            << "Usage: read_htdb_record_value FILENAME KEY VALUE_SIZE [OFFSET]"
            << std::endl;
        return 0;
    }

    data_chunk key_data;
    const std::string filename = argv[1];

    if (!decode_base16(key_data, argv[2]))
    {
        std::cerr << "key data is not valid" << std::endl;
        return -1;
    }

    const auto value_size = lexical_cast<size_t>(argv[3]);

    file_offset offset = 0;
    if (argc == 5)
        offset = lexical_cast<file_offset>(argv[4]);

    memory_map file(filename);
    BITCOIN_ASSERT(file.open());

    record_hash_table_header header(file, buckets);
    BITCOIN_ASSERT(header.start());

    const auto record_size = key_data.size() + 4 + value_size;
    record_manager manager(file, offset + 4 + 4 * header.size(), record_size);
    const auto result = manager.start();
    BITCOIN_ASSERT(result);

    memory_ptr record = nullptr;
    if (key_data.size() == 32)
    {
        record = get<32>(header, manager, key_data);
    }
    else if (key_data.size() == 4)
    {
        record = get<4>(header, manager, key_data);
    }
    else
    {
        std::cerr << "read_htdb_record_value: unsupported KEY size, "
            << "use 4 or 32." << std::endl;
        return -1;
    }

    if (!record)
    {
        std::cerr << "read_htdb_record_value: no record found" << std::endl;
        return -2;
    }

    const auto memory = REMAP_ADDRESS(record);
    data_chunk data(memory, memory + value_size);
    std::cout << encode_base16(data) << std::endl;
    return 0;
}

