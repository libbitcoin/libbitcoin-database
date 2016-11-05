#include <iostream>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

static const size_t buckets = 1000;

template <typename IndexType, typename ValueType>
int show_array(const std::string& filename)
{
    memory_map file(filename);
    hash_table_header<IndexType, ValueType> table(file, buckets);
    const auto result = table.start();
    BITCOIN_ASSERT(result);

    for (IndexType i = 0; i < table.size(); ++i)
    {
        std::string output;
        const auto value = table.read(i);

        if (value != table.empty)
            output = lexical_cast<std::string>(value);

        std::cout << i << ": " << output << std::endl;
    }

    return 0;
}

// Show the full contents of a hash table header (array).
int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: show_array FILENAME VALUE_SIZE"
            << std::endl;
        return 0;
    }

    const std::string filename = argv[1];
    const std::string value_size = argv[2];

    if (value_size == "4")
        return show_array<uint32_t, uint32_t>(filename);

    if (value_size == "8")
        return show_array<uint32_t, uint64_t>(filename);

    std::cerr << "show_array: unsupported VALUE_SIZE, use 4 or 8."
        << std::endl;
    return -1;
}
