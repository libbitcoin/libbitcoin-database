#include <iostream>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace bc;
using namespace bc::database;

void show_usage()
{
    std::cerr << "Usage: mmr_create KEY_SIZE VALUE_SIZE "
        "MAP_FILENAME ROWS_FILENAME [BUCKETS]" << std::endl;
}

template <size_t KeySize>
void mmr_create(const size_t value_size,
    const std::string& map_filename, const std::string& rows_filename,
    const array_index buckets)
{
    const auto header_size = record_hash_table_header_size(buckets);

    data_base::touch_file(map_filename);
    memory_map ht_file(map_filename);
    BITCOIN_ASSERT(ht_file.data());
    ht_file.resize(header_size + minimum_records_size);

    htdb_record_header header(ht_file, 0);
    header.create(buckets);
    header.start();

    typedef byte_array<KeySize> hash_type;
    const size_t record_size = hash_table_multimap_record_size<hash_type>();
    BITCOIN_ASSERT(record_size == KeySize + 4 + 4);
    const file_offset records_start = header_size;

    record_manager alloc(ht_file, records_start, record_size);
    alloc.create();
    alloc.start();

    record_hash_table<hash_type> ht(header, alloc, "test");

    data_base::touch_file(rows_filename);
    memory_map lrs_file(rows_filename);
    BITCOIN_ASSERT(lrs_file.data());
    lrs_file.resize(minimum_records_size);
    const size_t lrs_record_size = linked_record_offset + value_size;
    record_manager recs(lrs_file, 0, lrs_record_size);
    recs.create();

    recs.start();
    record_list lrs(recs);

    record_multimap<hash_type> multimap(ht, lrs, "test");
}

int main(int argc, char** argv)
{
    if (argc != 5 && argc != 6)
    {
        show_usage();
        return -1;
    }
    const size_t key_size = boost::lexical_cast<size_t>(argv[1]);
    const size_t value_size = boost::lexical_cast<size_t>(argv[2]);
    const std::string map_filename = argv[3];
    const std::string rows_filename = argv[4];
    array_index buckets = 100;
    if (argc == 6)
        buckets = boost::lexical_cast<array_index>(argv[5]);
    if (key_size == 4)
        mmr_create<4>(value_size, map_filename, rows_filename, buckets);
    else if (key_size == 20)
        mmr_create<20>(value_size, map_filename, rows_filename, buckets);
    else if (key_size == 32)
        mmr_create<32>(value_size, map_filename, rows_filename, buckets);
    return 0;
}

