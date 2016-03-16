#include <iostream>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

int main(int argc, char** argv)
{
    if (argc != 3 && argc != 4)
    {
        std::cerr << "Usage: show_records FILENAME RECORD_SIZE [OFFSET]"
            << std::endl;
        return 0;
    }

    const std::string filename = argv[1];
    const size_t record_size = lexical_cast<size_t>(argv[2]);

    file_offset offset = 0;
    if (argc == 4)
        offset = boost::lexical_cast<file_offset>(argv[3]);

    memory_map file(filename);
    record_manager manager(file, offset, record_size);
    manager.start();

    for (array_index i = 0; i < manager.count(); ++i)
    {
        const auto memory = manager.get(i);
        const auto buffer = REMAP_ADDRESS(memory);
        data_chunk data(buffer, buffer + record_size);
        std::cout << i << ": " << encode_base16(data) << std::endl;
    }

    return 0;
}
