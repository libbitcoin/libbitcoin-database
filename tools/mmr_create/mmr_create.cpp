#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

void show_usage()
{
    std::cerr << "Usage: mmr_create KEY_SIZE VALUE_SIZE BUCKETS "
        "MAP_FILENAME ROWS_FILENAME" << std::endl;
}

void show_key_size_error()
{
    std::cerr << "Invalid KEY_SIZE: use 4, 20 or 32." << std::endl;
}

template <size_t KeySize>
void mmr_create(const size_t value_size, const std::string& map_filename,
    const std::string& rows_filename,
    const array_index buckets)
{
    const auto header_size = record_hash_table_header_size(buckets);

    store::create(map_filename);
    memory_map ht_file(map_filename);
    auto result = ht_file.open();
    BITCOIN_ASSERT(result);

    ht_file.resize(header_size + minimum_records_size);
    record_hash_table_header header(ht_file, buckets);

    result = header.create();
    BITCOIN_ASSERT(result);

    result = header.start();
    BITCOIN_ASSERT(result);

    typedef byte_array<KeySize> hash_type;
    const size_t record_size = hash_table_multimap_record_size<hash_type>();
    BITCOIN_ASSERT(record_size == KeySize + 4 + 4);

    const file_offset records_start = header_size;
    record_manager alloc(ht_file, records_start, record_size);
    result = alloc.create();
    BITCOIN_ASSERT(result);

    result = alloc.start();
    BITCOIN_ASSERT(result);

    record_hash_table<hash_type> ht(header, alloc);
    store::create(rows_filename);
    memory_map lrs_file(rows_filename);
    result = lrs_file.open();
    BITCOIN_ASSERT(result);

    lrs_file.resize(minimum_records_size);
    const size_t lrs_record_size = record_list_offset + value_size;
    record_manager recs(lrs_file, 0, lrs_record_size);

    result = recs.create();
    BITCOIN_ASSERT(result);

    result = recs.start();
    BITCOIN_ASSERT(result);

    record_list lrs(recs);
    record_multimap<hash_type> multimap(ht, lrs);
}

int main(int argc, char** argv)
{
    if (argc != 6)
    {
        show_usage();
        return -1;
    }

    const auto key_size = lexical_cast<size_t>(argv[1]);
    const auto value_size = lexical_cast<size_t>(argv[2]);
    const auto buckets = lexical_cast<array_index>(argv[3]);
    const std::string map_filename = argv[4];
    const std::string rows_filename = argv[5];

    if (key_size == 4)
        mmr_create<4>(value_size, map_filename, rows_filename, buckets);
    else if (key_size == 20)
        mmr_create<20>(value_size, map_filename, rows_filename, buckets);
    else if (key_size == 32)
        mmr_create<32>(value_size, map_filename, rows_filename, buckets);
    else
    {
        show_key_size_error();
        return -1;
    }

    return 0;
}

