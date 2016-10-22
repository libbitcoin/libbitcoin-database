#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

void show_help()
{
    std::cout << "Usage: block_db COMMAND MAP ROWS [ARGS]" << std::endl;
    std::cout << std::endl;
    std::cout << "The most commonly used block_db commands are:" << std::endl;
    std::cout << "  initialize_new  " << "Create a new block_database" << std::endl;
    std::cout << "  get             " << "Fetch block by height or hash" << std::endl;
    std::cout << "  store           " << "Store a block" << std::endl;
    std::cout << "  unlink          " << "Unlink series of blocks from a height" << std::endl;
    std::cout << "  last_height     " << "Show last block height in current chain" << std::endl;
    std::cout << "  help            " << "Show help for commands" << std::endl;
}

void show_command_help(const std::string& command)
{
    if (command == "initialize_new")
    {
        std::cout << "Usage: block_db " << command << " MAP ROWS "
            << "" << std::endl;
    }
    else if (command == "get")
    {
        std::cout << "Usage: block_db " << command << " MAP ROWS "
            << "HEIGHT (or) HASH" << std::endl;
    }
    else if (command == "store")
    {
        std::cout << "Usage: block_db " << command << " MAP ROWS "
            << "BLOCK_DATA" << std::endl;
    }
    else if (command == "unlink")
    {
        std::cout << "Usage: block_db " << command << " MAP ROWS "
            << "FROM_HEIGHT" << std::endl;
    }
    else if (command == "last_height")
    {
        std::cout << "Usage: block_db " << command << " MAP ROWS "
            << std::endl;
    }
    else
    {
        std::cout << "No help available for " << command << std::endl;
    }
}

template <typename Point>
bool parse_point(Point& point, const std::string& arg)
{
    std::vector<std::string> tokens;
    split(tokens, arg, is_any_of(":"));

    if (tokens.size() != 2)
    {
        std::cerr << "block_db: bad point provided." << std::endl;
        return false;
    }

    const std::string& hexadecimal = tokens[0];
    if (!decode_hash(point.hash, hexadecimal))
    {
        std::cerr << "block_db: bad point provided." << std::endl;
        return false;
    }

    const std::string& index = tokens[1];
    try
    {
        point.index = lexical_cast<uint32_t>(index);
    }
    catch (const bad_lexical_cast&)
    {
        std::cerr << "block_db: bad point provided." << std::endl;
        return false;
    }

    return true;
}

bool parse_key(short_hash& key, const std::string& arg)
{
    const wallet::payment_address address(arg);

    if (!address)
    {
        std::cerr << "block_db: bad KEY." << std::endl;
        return false;
    }

    key = address.hash();
    return true;
}

template <typename Uint>
bool parse_uint(Uint& value, const std::string& arg)
{
    try
    {
        value = lexical_cast<Uint>(arg);
    }
    catch (const bad_lexical_cast&)
    {
        std::cerr << "block_db: bad value provided." << std::endl;
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
    typedef std::vector<std::string> string_list;

    if (argc < 2)
    {
        show_help();
        return -1;
    }

    const std::string command = argv[1];

    if (command == "help" || command == "-h" || command == "--help")
    {
        if (argc == 3)
        {
            show_command_help(argv[2]);
            return 0;
        }
        show_help();
        return 0;
    }

    if (argc < 4)
    {
        show_command_help(command);
        return -1;
    }

    string_list args;
    const std::string map_filename = argv[2];
    const std::string rows_filename = argv[3];

    for (int i = 4; i < argc; ++i)
        args.push_back(argv[i]);

    if (command == "initialize_new")
    {
        store::create(map_filename);
        store::create(rows_filename);
    }

    block_database db(map_filename, rows_filename, 1000, 50);

    if (command == "initialize_new")
    {
        const auto result = db.create();
        BITCOIN_ASSERT(result);
    }
    else if (command == "get")
    {
        if (args.size() != 1)
        {
            show_command_help(command);
            return -1;
        }

        const auto result = db.open();
        BITCOIN_ASSERT(result);
        std::shared_ptr<block_result> block_data;

        try
        {
            size_t height = lexical_cast<size_t>(args[0]);
            block_data = std::make_shared<block_result>(db.get(height));
        }
        catch (const bad_lexical_cast&)
        {
            hash_digest hash;
            if (!decode_hash(hash, args[0]))
            {
                std::cerr << "Couldn't read index value." << std::endl;
                return -1;
            }

            block_data = std::make_shared<block_result>(db.get(hash));
        }

        if (!block_data)
        {
            std::cout << "Not found!" << std::endl;
            return -1;
        }

        const auto block_header = block_data->header();
        const auto txs_size = block_data->transaction_count();
        const auto merkle = encode_hash(block_header.merkle());
        const auto previous = encode_hash(block_header.previous_block_hash());

        // Show details.
        std::cout << "height: " << block_data->height() << std::endl;
        std::cout << "hash: " << encode_hash(block_header.hash()) << std::endl;
        std::cout << "version: " << block_header.version() << std::endl;
        std::cout << "previous: " << previous << std::endl;
        std::cout << "merkle: " << merkle << std::endl;
        std::cout << "timestamp: " << block_header.timestamp() << std::endl;
        std::cout << "bits: " << block_header.bits() << std::endl;
        std::cout << "nonce: " << block_header.nonce() << std::endl;

        if (txs_size > 0)
        {
            std::cout << "Transactions:" << std::endl;

            for (size_t i = 0; i < txs_size; ++i)
                std::cout << "  "
                    << encode_hash(block_data->transaction_hash(i))
                    << std::endl;
        }
        else
        {
            std::cout << "No transactions" << std::endl;
        }
    }
    else if (command == "store")
    {
        if (args.size() != 1)
        {
            show_command_help(command);
            return -1;
        }

        data_chunk data;
        if (!decode_base16(data, args[0]))
        {
            std::cerr << "block_db: BLOCK_DATA is not valid" << std::endl;
            return -1;
        }

        if (data.size() < 80)
        {
            std::cerr << "block_db: BLOCK_DATA must be greater than 80 bytes"
                << std::endl;
            return -1;
        }

        chain::block block;
        if (block.from_data(data))
            throw end_of_stream();

        const auto result = db.open();
        BITCOIN_ASSERT(result);

        size_t top;
        /* bool */ db.top(top);

        db.store(block, top);
        db.synchronize();
    }
    else if (command == "unlink")
    {
        if (args.size() != 1)
        {
            show_command_help(command);
            return -1;
        }

        size_t from_height = 0;
        if (!parse_uint(from_height, args[0]))
            return -1;

        const auto result = db.open();
        BITCOIN_ASSERT(result);

        db.unlink(from_height);
        db.synchronize();
    }
    else if (command == "last_height")
    {
        const auto result = db.open();
        BITCOIN_ASSERT(result);

        size_t height;

        if (!db.top(height))
        {
            std::cout << "No blocks exist." << std::endl;
            return -2;
        }

        std::cout << height << std::endl;
    }
    else
    {
        std::cout << "block_db: '" << command
            << "' is not a block_db command. "
            << "See 'block_db --help'." << std::endl;
        return -1;
    }

    return 0;
}

