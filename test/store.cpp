/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <boost/test/unit_test.hpp>

#include <bitcoin/database.hpp>
#include "utility/utility.hpp"

using namespace bc;
using namespace bc::database;

// Test directory
#define DIRECTORY "store"

class store_accessor
  : public store
{
public:
    store_accessor(const path& prefix, bool indexes=false, bool flush=false,
        bool result=true)
      : store(prefix, indexes, flush), result_(result)
    {
    }

    virtual bool flush() const { return result_; }

private:
    bool result_;
};

struct store_directory_setup_fixture
{
    store_directory_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }
};

BOOST_FIXTURE_TEST_SUITE(store_tests, store_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(store__construct__flush_each_write_false__expected)
{
    store_accessor store("");
    BOOST_REQUIRE(!store.flush_each_write());
}

BOOST_AUTO_TEST_CASE(store__construct__flush_each_write_true__expected)
{
    store_accessor store("", false, true);
    BOOST_REQUIRE(store.flush_each_write());
}

using namespace boost::filesystem;
static bool create_file(const path& file_path)
{
    // Disallow create with existing file.
    if (bc::ifstream(file_path.string()).good())
        return false;

    bc::ofstream file(file_path.string());

    if (!file.good())
        return false;

    // Write one byte so file is nonzero size (for memory map validation).
    file.put('x');
    return true;
}

BOOST_AUTO_TEST_CASE(store__construct__no_indexes__expected_files)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory);

    static const std::string block_table = directory + "/" + store::BLOCK_TABLE;
    static const std::string candidate_index = directory + "/" + store::CANDIDATE_INDEX;
    static const std::string confirmed_index = directory + "/" + store::CONFIRMED_INDEX;
    static const std::string tx_index = directory + "/" + store::TRANSACTION_INDEX;
    static const std::string tx_table = directory + "/" + store::TRANSACTION_TABLE;
    static const std::string address_table = directory + "/" + store::ADDRESS_TABLE;
    static const std::string address_rows = directory + "/" + store::ADDRESS_ROWS;

    BOOST_REQUIRE(!test::exists(block_table));
    BOOST_REQUIRE(!test::exists(candidate_index));
    BOOST_REQUIRE(!test::exists(confirmed_index));
    BOOST_REQUIRE(!test::exists(tx_index));
    BOOST_REQUIRE(!test::exists(tx_table));
    BOOST_REQUIRE(!test::exists(address_table));
    BOOST_REQUIRE(!test::exists(address_rows));

    BOOST_REQUIRE(store.create());

    BOOST_REQUIRE(test::exists(block_table));
    BOOST_REQUIRE(test::exists(candidate_index));
    BOOST_REQUIRE(test::exists(confirmed_index));
    BOOST_REQUIRE(test::exists(tx_index));
    BOOST_REQUIRE(test::exists(tx_table));
    BOOST_REQUIRE(!test::exists(address_table));
    BOOST_REQUIRE(!test::exists(address_rows));

    BOOST_REQUIRE(store.close());
}

BOOST_AUTO_TEST_CASE(store__construct__indexes__expected_files)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory, true);

    static const std::string block_table = directory + "/" + store::BLOCK_TABLE;
    static const std::string candidate_index = directory + "/" + store::CANDIDATE_INDEX;
    static const std::string confirmed_index = directory + "/" + store::CONFIRMED_INDEX;
    static const std::string tx_index = directory + "/" + store::TRANSACTION_INDEX;
    static const std::string tx_table = directory + "/" + store::TRANSACTION_TABLE;
    static const std::string address_table = directory + "/" + store::ADDRESS_TABLE;
    static const std::string address_rows = directory + "/" + store::ADDRESS_ROWS;

    BOOST_REQUIRE(!test::exists(block_table));
    BOOST_REQUIRE(!test::exists(candidate_index));
    BOOST_REQUIRE(!test::exists(confirmed_index));
    BOOST_REQUIRE(!test::exists(tx_index));
    BOOST_REQUIRE(!test::exists(tx_table));
    BOOST_REQUIRE(!test::exists(address_table));
    BOOST_REQUIRE(!test::exists(address_rows));

    BOOST_REQUIRE(store.create());

    BOOST_REQUIRE(test::exists(block_table));
    BOOST_REQUIRE(test::exists(candidate_index));
    BOOST_REQUIRE(test::exists(confirmed_index));
    BOOST_REQUIRE(test::exists(tx_index));
    BOOST_REQUIRE(test::exists(tx_table));
    BOOST_REQUIRE(test::exists(address_table));
    BOOST_REQUIRE(test::exists(address_rows));

    BOOST_REQUIRE(store.close());
}

BOOST_AUTO_TEST_CASE(store__construct__exclusive_lock__expected_files)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory);

    static const std::string exclusive_lock = directory + "/" + store::EXCLUSIVE_LOCK;
    BOOST_REQUIRE(!test::exists(exclusive_lock));
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(!test::exists(exclusive_lock));
    BOOST_REQUIRE(store.open());
    BOOST_REQUIRE(test::exists(exclusive_lock));
    BOOST_REQUIRE(store.close());
    BOOST_REQUIRE(!test::exists(exclusive_lock));
}

BOOST_AUTO_TEST_CASE(store__construct__global_flush_lock__expected_files)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory);

    static const std::string flush_lock = directory + "/" + store::FLUSH_LOCK;
    BOOST_REQUIRE(!test::exists(flush_lock));
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(!test::exists(flush_lock));
    BOOST_REQUIRE(store.open());
    BOOST_REQUIRE(test::exists(flush_lock));

    BOOST_REQUIRE(store.begin_write());
    BOOST_REQUIRE(test::exists(flush_lock));
    BOOST_REQUIRE(store.end_write());
    BOOST_REQUIRE(test::exists(flush_lock));
    BOOST_REQUIRE(store.begin_write());
    BOOST_REQUIRE(test::exists(flush_lock));
    BOOST_REQUIRE(store.end_write());
    BOOST_REQUIRE(test::exists(flush_lock));

    BOOST_REQUIRE(store.close());
    BOOST_REQUIRE(!test::exists(flush_lock));
}

BOOST_AUTO_TEST_CASE(store__construct__local_flush_lock__expected_files)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory, false, true);

    static const std::string flush_lock = directory + "/" + store::FLUSH_LOCK;
    BOOST_REQUIRE(!test::exists(flush_lock));
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(!test::exists(flush_lock));
    BOOST_REQUIRE(store.open());
    BOOST_REQUIRE(!test::exists(flush_lock));

    BOOST_REQUIRE(store.begin_write());
    BOOST_REQUIRE(test::exists(flush_lock));
    BOOST_REQUIRE(store.end_write());
    BOOST_REQUIRE(!test::exists(flush_lock));
    BOOST_REQUIRE(store.begin_write());
    BOOST_REQUIRE(test::exists(flush_lock));
    BOOST_REQUIRE(store.end_write());
    BOOST_REQUIRE(!test::exists(flush_lock));

    BOOST_REQUIRE(store.close());
    BOOST_REQUIRE(!test::exists(flush_lock));
}

BOOST_AUTO_TEST_CASE(store__construct__unbalanced_begin_write__leaves_lock_after_close)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory, false, true);

    static const std::string flush_lock = directory + "/" + store::FLUSH_LOCK;
    BOOST_REQUIRE(!test::exists(flush_lock));
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(store.open());
    BOOST_REQUIRE(store.begin_write());
    BOOST_REQUIRE(store.close());
    BOOST_REQUIRE(test::exists(flush_lock));
}

BOOST_AUTO_TEST_CASE(store__construct__failed_flush__expected)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory, false, true, false);

    static const std::string flush_lock = directory + "/" + store::FLUSH_LOCK;
    BOOST_REQUIRE(!test::exists(flush_lock));
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(store.open());
    BOOST_REQUIRE(store.begin_write());
    BOOST_REQUIRE(!store.end_write());
    BOOST_REQUIRE(store.close());
    BOOST_REQUIRE(test::exists(flush_lock));
}

BOOST_AUTO_TEST_CASE(store__construct__unbalanced_end_write__success)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory, false, true);

    static const std::string flush_lock = directory + "/" + store::FLUSH_LOCK;
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(store.open());
    BOOST_REQUIRE(store.end_write());
    BOOST_REQUIRE(!test::exists(flush_lock));
}

BOOST_AUTO_TEST_CASE(store__open__before_create_existing_directory__success)
{
    static const std::string directory = DIRECTORY;
    store_accessor store(directory);
    BOOST_REQUIRE(store.open());
}

BOOST_AUTO_TEST_CASE(store__open__before_create_missing_directory__failure)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory);
    BOOST_REQUIRE(!store.open());
}

BOOST_AUTO_TEST_CASE(store__open__after_create_missing_directory__success)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory);
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(store.open());
}

BOOST_AUTO_TEST_CASE(store__close__before_create__success)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory);
    BOOST_REQUIRE(store.close());
}

BOOST_AUTO_TEST_CASE(store__close__before_open__idempotent)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory);
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(store.close());
}

BOOST_AUTO_TEST_CASE(store__create__twice__false)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory);
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(!store.create());
}

BOOST_AUTO_TEST_CASE(store__create__exists__false)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory);
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(store.close());
    BOOST_REQUIRE(!store.create());
}

BOOST_AUTO_TEST_CASE(store__open__twice__idempotent)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory);
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(store.open());
    BOOST_REQUIRE(!store.open());
}

BOOST_AUTO_TEST_CASE(store__close__twice__idempotent)
{
    static const std::string directory = DIRECTORY "/" + TEST_NAME;
    store_accessor store(directory);
    BOOST_REQUIRE(store.create());
    BOOST_REQUIRE(store.open());
    BOOST_REQUIRE(store.close());
    BOOST_REQUIRE(store.close());
}

BOOST_AUTO_TEST_SUITE_END()
