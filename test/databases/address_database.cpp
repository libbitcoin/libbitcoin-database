/////**
//// * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
//// *
//// * This file is part of libbitcoin.
//// *
//// * This program is free software: you can redistribute it and/or modify
//// * it under the terms of the GNU Affero General Public License as published by
//// * the Free Software Foundation, either version 3 of the License, or
//// * (at your option) any later version.
//// *
//// * This program is distributed in the hope that it will be useful,
//// * but WITHOUT ANY WARRANTY; without even the implied warranty of
//// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// * GNU Affero General Public License for more details.
//// *
//// * You should have received a copy of the GNU Affero General Public License
//// * along with this program.  If not, see <http://www.gnu.org/licenses/>.
//// */
////#include <boost/test/unit_test.hpp>
////
////#include <boost/filesystem.hpp>
////#include <bitcoin/database.hpp>
////#include "../utility/utility.hpp"
////
////using namespace boost::system;
////using namespace boost::filesystem;
////using namespace bc;
////using namespace bc::chain;
////using namespace bc::database;
////
////#define DIRECTORY "address_database"
////
////struct address_database_directory_setup_fixture
////{
////    address_database_directory_setup_fixture()
////    {
////        test::clear_path(DIRECTORY);
////    }
////};
////
////BOOST_FIXTURE_TEST_SUITE(database_tests, address_database_directory_setup_fixture)
////
////BOOST_AUTO_TEST_CASE(address_database__test)
////{
////    const short_hash key1 = base16_literal("a006500b7ddfd568e2b036c65a4f4d6aaa0cbd9b");
////    output_point out11{ hash_literal("4129e76f363f9742bc98dd3d40c99c9066e4d53b8e10e5097bd6f7b5059d7c53"), 110 };
////    const size_t out_h11 = 110;
////    const uint64_t value11 = 4;
////    output_point out12{ hash_literal("eefa5d23968584be9d8d064bcf99c24666e4d53b8e10e5097bd6f7b5059d7c53"), 4 };
////    const size_t out_h12 = 120;
////    const uint64_t value12 = 8;
////    output_point out13{ hash_literal("4129e76f363f9742bc98dd3d40c99c90eefa5d23968584be9d8d064bcf99c246"), 8 };
////    const size_t out_h13 = 222;
////    const uint64_t value13 = 6;
////
////    input_point spend11{ hash_literal("4742b3eac32d35961f9da9d42d495ff1d90aba96944cac3e715047256f7016d1"), 0 };
////    const size_t spend_h11 = 115;
////    input_point spend13{ hash_literal("3cc768bbaef30587c72c6eba8dbf6aeec4ef24172ae6fe357f2e24c2b0fa44d5"), 0 };
////    const size_t spend_h13 = 320;
////
////    const short_hash key2 = base16_literal("9c6b3bdaa612ceab88d49d4431ed58f26e69b90d");
////    output_point out21{ hash_literal("80d9e7012b5b171bf78e75b52d2d149580d9e7012b5b171bf78e75b52d2d1495"), 9 };
////    const size_t out_h21 = 3982;
////    const uint64_t value21 = 65;
////    output_point out22{ hash_literal("4742b3eac32d35961f9da9d42d495ff13cc768bbaef30587c72c6eba8dbf6aee"), 0 };
////    const size_t out_h22 = 78;
////    const uint64_t value22 = 9;
////
////    input_point spend22{ hash_literal("3cc768bbaef30587c72c6eba8dbfffffc4ef24172ae6fe357f2e24c2b0fa44d5"), 0 };
////    const size_t spend_h22 = 900;
////
////    const short_hash key3 = base16_literal("3eb84f6a98478e516325b70fecf9903e1ce7528b");
////    output_point out31{ hash_literal("d90aba96944cac3e715047256f7016d1d90aba96944cac3e715047256f7016d1"), 0 };
////    const size_t out_h31 = 378;
////    const uint64_t value31 = 34;
////
////    const short_hash key4 = base16_literal("d60db39ca8ce4caf0f7d2b7d3111535d9543473f");
////    ////output_point out42{ hash_literal("aaaaaaaaaaacac3e715047256f7016d1d90aaa96944cac3e715047256f7016d1"), 0};
////    const size_t out_h41 = 74448;
////    const uint64_t value41 = 990;
////
////    test::create(DIRECTORY "/address_table");
////    test::create(DIRECTORY "/address_rows");
////    address_database db(DIRECTORY "/address_table", DIRECTORY "/address_rows", 1000, 50);
////    BOOST_REQUIRE(db.create());
////    db.store(key1, { out_h11, out11, value11 });
////    db.store(key1, { out_h12, out12, value12 });
////    db.store(key1, { out_h13, out13, value13 });
////    db.store(key1, { spend_h11, spend11, out11.checksum() });
////    db.store(key1, { spend_h13, spend13, out13.checksum() });
////    db.store(key2, { out_h21, out21, value21 });
////    db.store(key2, { out_h22, out22, value22 });
////
////    auto result1 = db.get(key1);
////    BITCOIN_ASSERT(result1);
////
////    auto it1 = result1.begin();
////    BITCOIN_ASSERT(it != result1.end());
////    auto entry1_0 = *it1;
////
////    BOOST_REQUIRE(entry1_0.is_input());
////    BOOST_REQUIRE(entry1_0.point().hash() == spend13.hash());
////    BOOST_REQUIRE(entry1_0.point().index() == spend13.index());
////    BOOST_REQUIRE(entry1_0.height() == spend_h13);
////    BOOST_REQUIRE(entry1_0.data() == out13.checksum());
////
////    BITCOIN_ASSERT(++it1 != result1.end());
////    auto entry1_1 = *it1;
////
////    BOOST_REQUIRE(entry1_1.is_input());
////    BOOST_REQUIRE(entry1_1.point().hash() == spend11.hash());
////    BOOST_REQUIRE(entry1_1.point().index() == spend11.index());
////    BOOST_REQUIRE(entry1_1.height() == spend_h11);
////    BOOST_REQUIRE(entry1_1.data() == out11.checksum());
////
////    BITCOIN_ASSERT(++it1 != result1.end());
////    auto entry1_2 = *it1;
////
////    BOOST_REQUIRE(entry1_2.is_output());
////    BOOST_REQUIRE(entry1_2.point().hash() == out13.hash());
////    BOOST_REQUIRE(entry1_2.point().index() == out13.index());
////    BOOST_REQUIRE(entry1_2.height() == out_h13);
////    BOOST_REQUIRE(entry1_2.data() == value13);
////
////    BITCOIN_ASSERT(++it1 != result1.end());
////    auto entry1_3 = *it1;
////
////    BOOST_REQUIRE(entry1_3.is_output());
////    BOOST_REQUIRE(entry1_3.point().hash() == out12.hash());
////    BOOST_REQUIRE(entry1_3.point().index() == out12.index());
////    BOOST_REQUIRE(entry1_3.height() == out_h12);
////    BOOST_REQUIRE(entry1_3.data() == value12);
////
////    BITCOIN_ASSERT(++it1 != result1.end());
////    auto entry1_4 = *it1;
////
////    BOOST_REQUIRE(entry1_4.is_valid());
////    BOOST_REQUIRE(entry1_4.is_output());
////    BOOST_REQUIRE(entry1_4.point().hash() == out11.hash());
////    BOOST_REQUIRE(entry1_4.point().index() == out11.index());
////    BOOST_REQUIRE(entry1_4.height() == out_h11);
////    BOOST_REQUIRE(entry1_4.data() == value11);
////
////    auto result2 = db.get(key2);
////    auto it2 = result2.begin();
////
////    BITCOIN_ASSERT(it2 != result2.end());
////    auto entry_1_0 = *it2;
////    BOOST_REQUIRE(entry_1_0.is_output());
////
////    BITCOIN_ASSERT(++it2 != result2.end());
////    auto entry_1_1 = *it2;
////    BOOST_REQUIRE(entry_1_1.is_output());
////
////    db.store(key2, { spend_h22, spend22, out22.checksum() });
////    auto result3 = db.get(key2);
////    auto it3 = result3.begin();
////
////    BITCOIN_ASSERT(it3 != result3.end());
////    auto entry_3_0 = *it3;
////
////    BOOST_REQUIRE(entry_3_0.is_output());
////    BOOST_REQUIRE(entry_3_0.point().hash() == out21.hash());
////    BOOST_REQUIRE(entry_3_0.point().index() == out21.index());
////    BOOST_REQUIRE(entry_3_0.height() == out_h21);
////    BOOST_REQUIRE(entry_3_0.data() == value21);
////
////    BITCOIN_ASSERT(++it3 != result3.end());
////    auto entry_3_1 = *it3;
////
////    BOOST_REQUIRE(entry_3_1.is_output());
////    BOOST_REQUIRE(entry_3_1.point().hash() == out22.hash());
////    BOOST_REQUIRE(entry_3_1.point().index() == out22.index());
////    BOOST_REQUIRE(entry_3_1.height() == out_h22);
////    BOOST_REQUIRE(entry_3_1.data() == value22);
////
////    BITCOIN_ASSERT(++it3 != result3.end());
////    auto entry_3_2 = *it3;
////
////    BOOST_REQUIRE(entry_3_2.is_input());
////    BOOST_REQUIRE(entry_3_2.point().hash() == spend22.hash());
////    BOOST_REQUIRE(entry_3_2.point().index() == spend22.index());
////    BOOST_REQUIRE(entry_3_2.height() == spend_h22);
////    BOOST_REQUIRE(entry_3_2.data() == out22.checksum());
////
////    db.pop(key2);
////    auto result4 = db.get(key2);
////    auto it4 = result4.begin();
////
////    BITCOIN_ASSERT(it4 != result4.end());
////    auto entry_4_0 = *it4;
////    BOOST_REQUIRE(entry_4_0.is_output());
////
////    BITCOIN_ASSERT(++it4 != result4.end());
////    auto entry_4_1 = *it4;
////    BOOST_REQUIRE(entry_4_1.is_output());
////
////    db.store(key3, { out_h31, out31, value31 });
////    auto result5 = db.get(key3);
////    auto it5 = result5.begin();
////    BITCOIN_ASSERT(it5 != result5.end());
////    BITCOIN_ASSERT(++it5 == result5.end());
////
////    db.store(key4, { out_h41, out31, value41 });
////    auto result6 = db.get(key4);
////    auto it6 = result6.begin();
////    BITCOIN_ASSERT(it6 != result6.end());
////    BITCOIN_ASSERT(++it6 == result6.end());
////
////    db.pop(key3);
////    auto result7 = db.get(key3);
////    auto it7 = result7.begin();
////    BITCOIN_ASSERT(it7 == result7.end());
////
////    auto result8 = db.get(key4);
////    auto it8 = result8.begin();
////    BITCOIN_ASSERT(it8 != result8.end());
////    BITCOIN_ASSERT(++it8 == result8.end());
////
////    db.commit();
////}
////
////BOOST_AUTO_TEST_SUITE_END()
