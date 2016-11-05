#include <iostream>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

static const size_t buckets = 1000;

void show_usage()
{
    std::cerr << "Usage: mmr_lookup KEY VALUE_SIZE "
        "MAP_FILENAME ROWS_FILENAME" << std::endl;
}

void show_key_size_error()
{
    std::cerr << "Invalid KEY size: use 4, 20 or 32." << std::endl;
}

template <size_t KeySize>
int mmr_lookup(const data_chunk& key_data, const size_t value_size,
    const std::string& map_filename, const std::string& rows_filename)
{
    typedef byte_array<KeySize> hash_type;

    hash_type key;
    BITCOIN_ASSERT(key.size() == key_data.size());
    std::copy(key_data.begin(), key_data.end(), key.begin());

    memory_map ht_file(map_filename);
    BITCOIN_ASSERT(ht_file.open());

    record_hash_table_header header(ht_file, buckets);
    auto result = header.start();
    BITCOIN_ASSERT(result);

    const auto record_size = hash_table_multimap_record_size<hash_type>();
    BITCOIN_ASSERT(record_size == KeySize + 4 + 4);
    const auto header_size = record_hash_table_header_size(header.size());
    const file_offset records_start = header_size;

    record_manager ht_manager(ht_file, records_start, record_size);
    result = ht_manager.start();
    BITCOIN_ASSERT(result);

    memory_map lrs_file(rows_filename);
    BITCOIN_ASSERT(lrs_file.open());

    const auto lrs_record_size = record_list_offset + value_size;
    record_manager lrs_manager(lrs_file, 0, lrs_record_size);
    result = lrs_manager.start();
    BITCOIN_ASSERT(result);

    record_list lrs(lrs_manager);
    record_hash_table<hash_type> ht(header, ht_manager);
    record_multimap<hash_type> multimap(ht, lrs);
    record_multimap_iterable container(lrs, multimap.lookup(key));

    for (const array_index index: container)
    {
        std::cout << "Index: " << index << std::endl;
        const auto memory = lrs_manager.get(index);
        const auto record = REMAP_ADDRESS(memory) + record_list_offset;
        const data_chunk data(record, record + value_size);
        std::cout << encode_base16(data) << std::endl;
        std::cout << std::endl;
    }

    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        show_usage();
        return -1;
    }

    data_chunk key_data;
    if (!decode_base16(key_data, argv[1]))
    {
        std::cerr << "key data is not valid" << std::endl;
        return -1;
    }

    const auto value_size = lexical_cast<size_t>(argv[2]);
    const std::string map_file = argv[3];
    const std::string rows_file = argv[4];

    if (key_data.size() == 4)
        return mmr_lookup<4>(key_data, value_size, map_file, rows_file);

    if (key_data.size() == 20)
        return mmr_lookup<20>(key_data, value_size, map_file, rows_file);

    if (key_data.size() == 32)
        return mmr_lookup<32>(key_data, value_size, map_file, rows_file);

    show_key_size_error();
    return -1;
}

