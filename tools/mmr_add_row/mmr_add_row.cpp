#include <iostream>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace bc;
using namespace bc::database;

static const size_t buckets = 1000;

void show_usage()
{
    std::cerr << "Usage: mmr_add_row KEY VALUE "
        "MAP_FILENAME ROWS_FILENAME" << std::endl;
}

template <size_t KeySize>
int mmr_add_row(const data_chunk& key_data, const data_chunk& value,
    const std::string& map_filename, const std::string& rows_filename)
{
    typedef byte_array<KeySize> hash_type;
    hash_type key;
    BITCOIN_ASSERT(key.size() == key_data.size());
    std::copy(key_data.begin(), key_data.end(), key.begin());

    memory_map ht_file(map_filename);
    auto result = ht_file.open();
    BITCOIN_ASSERT(result);

    record_hash_table_header header(ht_file, buckets);
    result = header.start();
    BITCOIN_ASSERT(result);

    const size_t record_size = hash_table_multimap_record_size<hash_type>();
    BITCOIN_ASSERT(record_size == KeySize + 4 + 4);
    const size_t header_size = record_hash_table_header_size(header.size());
    const file_offset records_start = header_size;

    record_manager alloc(ht_file, records_start, record_size);
    result = alloc.start();
    BITCOIN_ASSERT(result);

    record_hash_table<hash_type> ht(header, alloc);

    memory_map lrs_file(rows_filename);
    result = lrs_file.open();
    BITCOIN_ASSERT(result);

    const size_t lrs_record_size = record_list_offset + value.size();
    record_manager recs(lrs_file, 0, lrs_record_size);
    result = recs.start();
    BITCOIN_ASSERT(result);

    record_list lrs(recs);
    record_multimap<hash_type> multimap(ht, lrs);
    auto write = [&value](memory_ptr data)
    {
        std::copy(value.begin(), value.end(), REMAP_ADDRESS(data));
    };
    multimap.add_row(key, write);

    alloc.sync();
    recs.sync();
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

    data_chunk value;
    if (!decode_base16(value, argv[2]))
    {
        std::cerr << "value is not valid" << std::endl;
        return -1;
    }

    const std::string map_filename = argv[3];
    const std::string rows_filename = argv[4];
    if (key_data.size() == 4)
        return mmr_add_row<4>(key_data, value, map_filename, rows_filename);
    if (key_data.size() == 20)
        return mmr_add_row<20>(key_data, value, map_filename, rows_filename);
    if (key_data.size() == 32)
        return mmr_add_row<32>(key_data, value, map_filename, rows_filename);
    return 0;
}

