#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::chain;
using namespace bc::database;

void show_help()
{
    std::cout << "Usage: spend_db COMMAND FILE [ARGS]" << std::endl;
    std::cout << std::endl;
    std::cout << "The most commonly used spend_db commands are:" << std::endl;
    std::cout << "  initialize_new  " << "Create a new history_database" << std::endl;
    std::cout << "  get             " << "Fetch spend by outpoint" << std::endl;
    std::cout << "  store           " << "Store a spend" << std::endl;
    std::cout << "  remove          " << "Remove a spend" << std::endl;
    std::cout << "  statinfo        " << "Show statistical info for the database" << std::endl;
    std::cout << "  help            " << "Show help for commands" << std::endl;
}

void show_command_help(const std::string& command)
{
    if (command == "initialize_new")
    {
        std::cout << "Usage: spend_db " << command << " FILE "
            << "" << std::endl;
    }
    else if (command == "get")
    {
        std::cout << "Usage: spend_db " << command << " FILE "
            << "OUTPOINT" << std::endl;
    }
    else if (command == "store")
    {
        std::cout << "Usage: spend_db " << command << " FILE "
            << "OUTPOINT SPEND" << std::endl;
    }
    else if (command == "remove")
    {
        std::cout << "Usage: spend_db " << command << " FILE "
            << "OUTPOINT" << std::endl;
    }
    else if (command == "statinfo")
    {
        std::cout << "Usage: spend_db " << command << " FILE "
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
        std::cerr << "spend_db: bad point provided." << std::endl;
        return false;
    }

    hash_digest hash;
    const std::string& hexadecimal = tokens[0];

    if (!decode_hash(hash, hexadecimal))
    {
        std::cerr << "spend_db: bad point provided." << std::endl;
        return false;
    }

    point.set_hash(hash);
    const auto& index_string = tokens[1];

    try
    {
        point.set_index(lexical_cast<uint32_t>(index_string));
    }
    catch (const bad_lexical_cast&)
    {
        std::cerr << "spend_db: bad point provided." << std::endl;
        return false;
    }

    return true;
}

bool parse_key(short_hash& key, const std::string& arg)
{
    const wallet::payment_address address(arg);

    if (!address)
    {
        std::cerr << "spend_db: bad KEY." << std::endl;
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
        std::cerr << "spend_db: bad value provided." << std::endl;
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

    if (argc < 3)
    {
        show_command_help(command);
        return -1;
    }

    string_list args;
    const std::string filename = argv[2];

    for (int i = 3; i < argc; ++i)
        args.push_back(argv[i]);

    if (command == "initialize_new")
        store::create(filename);

    spend_database db(filename, 1000, 50);

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

        output_point outpoint;

        if (!parse_point(outpoint, args[0]))
            return -1;

        const auto result = db.open();
        BITCOIN_ASSERT(result);

        const auto spend = db.get(outpoint);

        if (!spend.is_valid())
        {
            std::cout << "Not found!" << std::endl;
            return -1;
        }

        std::cout << encode_hash(spend.hash()) << ":" << spend.index()
            << std::endl;
    }
    else if (command == "store")
    {
        if (args.size() != 2)
        {
            show_command_help(command);
            return -1;
        }

        output_point outpoint;

        if (!parse_point(outpoint, args[0]))
            return -1;

        chain::input_point spend;

        if (!parse_point(spend, args[1]))
            return -1;

        const auto result = db.open();
        BITCOIN_ASSERT(result);

        db.store(outpoint, spend);
        db.synchronize();
    }
    else if (command == "remove")
    {
        if (args.size() != 1)
        {
            show_command_help(command);
            return -1;
        }

        chain::output_point outpoint;

        if (!parse_point(outpoint, args[0]))
            return -1;

        const auto result = db.open();
        BITCOIN_ASSERT(result);

        db.unlink(outpoint);
        db.synchronize();
    }
    else if (command == "statinfo")
    {
        if (!args.empty())
        {
            show_command_help(command);
            return -1;
        }

        const auto result = db.open();
        BITCOIN_ASSERT(result);

        auto info = db.statinfo();
        std::cout << "Buckets: " << info.buckets << std::endl;
        std::cout << "Total rows: " << info.rows << std::endl;
    }
    else
    {
        std::cout << "spend_db: '" << command
            << "' is not a spend_db command. "
            << "See 'spend_db --help'." << std::endl;

        return -1;
    }

    return 0;
}

