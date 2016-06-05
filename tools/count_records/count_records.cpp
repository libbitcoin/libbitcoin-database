#include <iostream>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::cerr << "Usage: count_records FILENAME RECORD_SIZE BUCKETS"
            << std::endl;
        return 0;
    }

    const std::string filename = argv[1];
    const auto record_size = lexical_cast<size_t>(argv[2]);
    const auto buckets = lexical_cast<file_offset>(argv[3]);

    memory_map file(filename);
    record_manager records(file, buckets, record_size);

    const auto result = records.start();
    BITCOIN_ASSERT(result);

    std::cout << records.count() << std::endl;

    return 0;
}

