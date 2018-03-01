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

#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>
#include "../utility/utility.hpp"

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::chain;
using namespace bc::database;

#define DIRECTORY "address_database"

struct address_database_directory_setup_fixture
{
    address_database_directory_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }
};

BOOST_FIXTURE_TEST_SUITE(database_tests, address_database_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(address_database__test)
{
    const short_hash key1 = base16_literal("a006500b7ddfd568e2b036c65a4f4d6aaa0cbd9b");
    static const payment_record output_11{ 65, 110, 4, true };
    static const payment_record output_12{ 238, 4, 8, true };
    static const payment_record output_13{ 65, 8, 6, true };
    static const payment_record input_11{ 71, 0, 0x0a, false };
    static const payment_record input_13{ 60, 0, 0x0b, false };

    const short_hash key2 = base16_literal("9c6b3bdaa612ceab88d49d4431ed58f26e69b90d");
    static const payment_record output_21{ 128, 9, 65, true };
    static const payment_record output_22{ 71, 0, 9, true };
    static const payment_record input_22{ 60, 0, 0x0c, false };

    const short_hash key3 = base16_literal("3eb84f6a98478e516325b70fecf9903e1ce7528b");
    static const payment_record output_31{ 217, 0, 34, true };

    const short_hash key4 = base16_literal("d60db39ca8ce4caf0f7d2b7d3111535d9543473f");
    static const payment_record output_41{ 170, 0, 7990, true };

    test::create(DIRECTORY "/address_table");
    test::create(DIRECTORY "/address_rows");
    address_database db(DIRECTORY "/address_table", DIRECTORY "/address_rows", 1000, 50);
    BOOST_REQUIRE(db.create());

    db.store(key1, output_11);
    db.store(key1, output_12);
    db.store(key1, output_13);
    db.store(key1, input_11);
    db.store(key1, input_13);
    db.store(key2, output_21);
    db.store(key2, output_22);

    auto result1 = db.get(key1);
    BOOST_REQUIRE(result1);

    auto it1 = result1.begin();
    BOOST_REQUIRE(it1 != result1.end());
    auto entry1_0 = *it1;

    BOOST_REQUIRE(!entry1_0.is_output());
    BOOST_REQUIRE(entry1_0.link() == input_13.link());
    BOOST_REQUIRE(entry1_0.point().index() == input_13.point().index());
    BOOST_REQUIRE(entry1_0.height() == input_13.height());
    BOOST_REQUIRE(entry1_0.data() == input_13.data());

    BOOST_REQUIRE(++it1 != result1.end());
    auto entry_1_1 = *it1;

    BOOST_REQUIRE(!entry_1_1.is_output());
    BOOST_REQUIRE(entry_1_1.link() == input_11.link());
    BOOST_REQUIRE(entry_1_1.point().index() == input_11.point().index());
    BOOST_REQUIRE(entry_1_1.height() == input_11.height());
    BOOST_REQUIRE(entry_1_1.data() == input_11.data());

    BOOST_REQUIRE(++it1 != result1.end());
    auto entry_1_2 = *it1;

    BOOST_REQUIRE(entry_1_2.is_output());
    BOOST_REQUIRE(entry_1_2.link() == output_13.link());
    BOOST_REQUIRE(entry_1_2.point().index() == output_13.point().index());
    BOOST_REQUIRE(entry_1_2.height() == output_13.height());
    BOOST_REQUIRE(entry_1_2.data() == output_13.data());

    BOOST_REQUIRE(++it1 != result1.end());
    auto entry_1_3 = *it1;

    BOOST_REQUIRE(entry_1_3.is_output());
    BOOST_REQUIRE(entry_1_3.link() == output_12.link());
    BOOST_REQUIRE(entry_1_3.point().index() == output_12.point().index());
    BOOST_REQUIRE(entry_1_3.height() == output_12.height());
    BOOST_REQUIRE(entry_1_3.data() == output_12.data());

    BOOST_REQUIRE(++it1 != result1.end());
    auto entry_1_4 = *it1;

    BOOST_REQUIRE(entry_1_4.is_valid());
    BOOST_REQUIRE(entry_1_4.is_output());
    BOOST_REQUIRE(entry_1_4.link() == output_11.link());
    BOOST_REQUIRE(entry_1_4.point().index() == output_11.point().index());
    BOOST_REQUIRE(entry_1_4.height() == output_11.height());
    BOOST_REQUIRE(entry_1_4.data() == output_11.data());

    auto result2 = db.get(key2);
    auto it2 = result2.begin();

    BOOST_REQUIRE(it2 != result2.end());
    auto entry_2_0 = *it2;
    BOOST_REQUIRE(entry_2_0.is_output());

    BOOST_REQUIRE(++it2 != result2.end());
    auto entry_2_1 = *it2;
    BOOST_REQUIRE(entry_2_1.is_output());

    db.store(key2, input_22);
    auto result3 = db.get(key2);
    auto it3 = result3.begin();

    BOOST_REQUIRE(it3 != result3.end());
    auto entry_3_0 = *it3;

    BOOST_REQUIRE(!entry_3_0.is_output());
    BOOST_REQUIRE_EQUAL(entry_3_0.link(), input_22.link());
    BOOST_REQUIRE_EQUAL(entry_3_0.point().index(), input_22.point().index());
    BOOST_REQUIRE_EQUAL(entry_3_0.height(), input_22.height());
    BOOST_REQUIRE_EQUAL(entry_3_0.data(), input_22.data());

    BOOST_REQUIRE(++it3 != result3.end());
    auto entry_3_1 = *it3;

    BOOST_REQUIRE(entry_3_1.is_output());
    BOOST_REQUIRE(entry_3_1.link() == output_22.link());
    BOOST_REQUIRE(entry_3_1.point().index() == output_22.point().index());
    BOOST_REQUIRE(entry_3_1.height() == output_22.height());
    BOOST_REQUIRE(entry_3_1.data() == output_22.data());

    BOOST_REQUIRE(++it3 != result3.end());
    auto entry_3_2 = *it3;

    BOOST_REQUIRE(entry_3_2.is_output());
    BOOST_REQUIRE(entry_3_2.link() == output_21.link());
    BOOST_REQUIRE(entry_3_2.point().index() == output_21.point().index());
    BOOST_REQUIRE(entry_3_2.height() == output_21.height());
    BOOST_REQUIRE(entry_3_2.data() == output_21.data());

    db.pop(key2);
    auto result4 = db.get(key2);
    auto it4 = result4.begin();

    BOOST_REQUIRE(it4 != result4.end());
    auto entry_4_0 = *it4;
    BOOST_REQUIRE(entry_4_0.is_output());

    BOOST_REQUIRE(++it4 != result4.end());
    auto entry_4_1 = *it4;
    BOOST_REQUIRE(entry_4_1.is_output());

    db.store(key3, output_31);
    auto result5 = db.get(key3);
    auto it5 = result5.begin();
    BOOST_REQUIRE(it5 != result5.end());
    BOOST_REQUIRE(++it5 == result5.end());

    db.store(key4, output_41);
    auto result6 = db.get(key4);
    auto it6 = result6.begin();
    BOOST_REQUIRE(it6 != result6.end());
    BOOST_REQUIRE(++it6 == result6.end());

    db.pop(key3);
    auto result7 = db.get(key3);
    auto it7 = result7.begin();
    BOOST_REQUIRE(it7 == result7.end());

    auto result8 = db.get(key4);
    auto it8 = result8.begin();
    BOOST_REQUIRE(it8 != result8.end());
    BOOST_REQUIRE(++it8 == result8.end());

    db.commit();
}

BOOST_AUTO_TEST_SUITE_END()
