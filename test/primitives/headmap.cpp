/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include "../test.hpp"
#include "../mocks/chunk_storage.hpp"

BC_AUTO_TEST_SUITE(headmap_tests)

using namespace system;

using link = linkage<2>;
using test_headmap = headmap<link, sizeof(uint16_t)>;
using small_link = linkage<3>;
using unaligned_headmap = headmap<small_link, small_link::size>;
using slab_link = linkage<5>;
using slab_headmap = headmap<slab_link, max_size_t>;
using value_link = linkage<3>;
using value_headmap = headmap<value_link, one, uint8_t>;

// Reports a stale size once, as a reader racing a truncate observes it.
class stale_storage
  : public test::chunk_storage
{
public:
    using test::chunk_storage::chunk_storage;

    size_t size() const NOEXCEPT override
    {
        const auto stale = stale_;
        stale_ = zero;
        return is_zero(stale) ? test::chunk_storage::size() : stale;
    }

    mutable size_t stale_{};
};

class little_slab
{
public:
    static constexpr size_t size = max_size_t;
    static constexpr slab_link count() NOEXCEPT { return sizeof(uint32_t); }

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_little_endian<uint32_t>();
        return source;
    }

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

// Opaque byte cell read.
class byte_get
{
public:
    static constexpr size_t size = one;

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_byte();
        return source;
    }

    uint8_t value{};
};

// Opaque byte cell overwrite.
class byte_put
{
public:
    static constexpr size_t size = one;

    bool from_data(database::reader& source) NOEXCEPT
    {
        source.skip_byte();
        return source;
    }

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_byte(value);
        return sink;
    }

    uint8_t value{};
};

// Opaque byte cell flag merge.
class flags_put
{
public:
    static constexpr size_t size = one;

    bool from_data(database::reader& source) NOEXCEPT
    {
        current = source.read_byte();
        return source;
    }

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_byte(bit_or<uint8_t>(current, flags));
        return sink;
    }

    uint8_t flags{};
    uint8_t current{};
};

// Two byte link cell overwrite.
class link_put
{
public:
    static constexpr size_t size = link::size;

    bool from_data(database::reader& source) NOEXCEPT
    {
        source.skip_bytes(link::size);
        return source;
    }

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian<link::integer>(value);
        return sink;
    }

    link::integer value{};
};

// Three byte link cell read.
class small_link_get
{
public:
    static constexpr size_t size = small_link::size;

    bool from_data(database::reader& source) NOEXCEPT
    {
        using integer = small_link::integer;
        value = source.read_little_endian<integer, small_link::size>();
        return source;
    }

    small_link::integer value{};
};

BC_AUTO_TEST_CASE(headmap__create__empty__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    test_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.verify());
    BC_REQUIRE(head_file.empty());
    BC_REQUIRE_EQUAL(instance.head_size(), zero);
    BC_REQUIRE_EQUAL(instance.body_size(), zero);
    BC_REQUIRE_EQUAL(instance.count(), 0u);
    BC_REQUIRE(instance.at(0).is_terminal());
}

BC_AUTO_TEST_CASE(headmap__create__buckets__filled_terminal)
{
    constexpr auto buckets = 3_size;
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    test_headmap instance{ head_store, buckets };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.verify());
    BC_REQUIRE_EQUAL(instance.head_size(), buckets * sizeof(uint16_t));
    BC_REQUIRE_EQUAL(instance.count(), 3u);
    BC_REQUIRE(instance.at(0).is_terminal());
    BC_REQUIRE(instance.at(1).is_terminal());
    BC_REQUIRE(instance.at(2).is_terminal());
    BC_REQUIRE(instance.at(3).is_terminal());
    BC_REQUIRE(!instance.create());
}

BC_AUTO_TEST_CASE(headmap__push__reserved__expected_at)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    test_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(!instance.push(link::terminal));
    BC_REQUIRE(instance.reserve(1u));
    BC_REQUIRE(instance.push(0x1122_u16));
    BC_REQUIRE(instance.reserve(1u));
    BC_REQUIRE(instance.push(0x3344_u16));
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(instance.at(0), 0x1122_u16);
    BC_REQUIRE_EQUAL(instance.at(1), 0x3344_u16);
    BC_REQUIRE(instance.at(2).is_terminal());
    BC_REQUIRE_EQUAL(head_file, base16_chunk("22114433"));
}

BC_AUTO_TEST_CASE(headmap__push__unreserved__false)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    test_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(!instance.push(0x1122_u16));
    BC_REQUIRE_EQUAL(instance.count(), 0u);
}

BC_AUTO_TEST_CASE(headmap__truncate__push__rewrites_bucket)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    test_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.reserve(2u));
    BC_REQUIRE(instance.push(0x1122_u16));
    BC_REQUIRE(instance.push(0x3344_u16));
    BC_REQUIRE(!instance.truncate(3u));
    BC_REQUIRE(instance.truncate(1u));
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    BC_REQUIRE(instance.at(1).is_terminal());
    BC_REQUIRE(instance.reserve(1u));
    BC_REQUIRE(instance.push(0x5566_u16));
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(instance.at(1), 0x5566_u16);
}

BC_AUTO_TEST_CASE(headmap__top__empty__terminal)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    test_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.top().is_terminal());
}

BC_AUTO_TEST_CASE(headmap__top__single_genesis__zero)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    test_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.reserve(1u));
    BC_REQUIRE(instance.push(0x0000_u16));
    BC_REQUIRE_EQUAL(instance.top(), 0x0000_u16);
}

BC_AUTO_TEST_CASE(headmap__top__many__last_pushed)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    test_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.reserve(3u));
    BC_REQUIRE(instance.push(0x1122_u16));
    BC_REQUIRE(instance.push(0x3344_u16));
    BC_REQUIRE(instance.push(0x5566_u16));
    BC_REQUIRE_EQUAL(instance.count(), 3u);
    BC_REQUIRE_EQUAL(instance.top(), 0x5566_u16);
}

BC_AUTO_TEST_CASE(headmap__top__truncated__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    test_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.reserve(3u));
    BC_REQUIRE(instance.push(0x1122_u16));
    BC_REQUIRE(instance.push(0x3344_u16));
    BC_REQUIRE(instance.push(0x5566_u16));
    BC_REQUIRE(instance.truncate(2u));
    BC_REQUIRE_EQUAL(instance.top(), 0x3344_u16);
    BC_REQUIRE(instance.truncate(1u));
    BC_REQUIRE_EQUAL(instance.top(), 0x1122_u16);
    BC_REQUIRE(instance.truncate(0u));
    BC_REQUIRE(instance.top().is_terminal());
}

BC_AUTO_TEST_CASE(headmap__top__truncate_race__popped_link)
{
    data_chunk head_file{};
    stale_storage head_store{ head_file };
    test_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.reserve(3u));
    BC_REQUIRE(instance.push(0x1122_u16));
    BC_REQUIRE(instance.push(0x3344_u16));
    BC_REQUIRE(instance.push(0x5566_u16));
    BC_REQUIRE(instance.truncate(2u));
    head_store.stale_ = 6u;
    BC_REQUIRE_EQUAL(instance.top(), 0x5566_u16);
    BC_REQUIRE_EQUAL(instance.top(), 0x3344_u16);
    BC_REQUIRE_EQUAL(instance.count(), 2u);
}

BC_AUTO_TEST_CASE(headmap__unaligned__push_at_truncate__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    unaligned_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.reserve(2u));
    BC_REQUIRE(instance.push(0x00112233_u32));
    BC_REQUIRE(instance.push(0x00445566_u32));
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(instance.at(0), 0x00112233_u32);
    BC_REQUIRE_EQUAL(instance.at(1), 0x00445566_u32);
    BC_REQUIRE(instance.at(2).is_terminal());
    BC_REQUIRE_EQUAL(head_file, base16_chunk("332211665544"));
    BC_REQUIRE(instance.truncate(1u));
    BC_REQUIRE(instance.at(1).is_terminal());
}

BC_AUTO_TEST_CASE(headmap__setup__lifecycle__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    test_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.reserve(1u));
    BC_REQUIRE(instance.push(0x1122_u16));
    BC_REQUIRE(instance.close());
    BC_REQUIRE(instance.backup());
    BC_REQUIRE(instance.restore());
    BC_REQUIRE(instance.verify());
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    BC_REQUIRE(!instance.get_fault());
}

// slab

BC_AUTO_TEST_CASE(headmap__slab_create__empty__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    slab_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.verify());
    BC_REQUIRE_EQUAL(instance.head_size(), zero);
    BC_REQUIRE_EQUAL(instance.body_size(), zero);
}

BC_AUTO_TEST_CASE(headmap__slab_put__allocated__expected_get)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    slab_headmap instance{ head_store };
    BC_REQUIRE(instance.create());

    const auto link = instance.allocate(little_slab::count());
    BC_REQUIRE(!link.is_terminal());
    BC_REQUIRE_EQUAL(link, 0u);
    BC_REQUIRE_EQUAL(instance.head_size(), sizeof(uint32_t));

    const little_slab in{ 0x11223344_u32 };
    BC_REQUIRE(instance.put(link, in));

    little_slab out{};
    BC_REQUIRE(instance.get(link, out));
    BC_REQUIRE_EQUAL(out.value, in.value);
}

// The link is a byte offset, as with a body slab.
BC_AUTO_TEST_CASE(headmap__slab_put__second_element__expected_get)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    slab_headmap instance{ head_store };
    BC_REQUIRE(instance.create());

    const auto first = instance.allocate(little_slab::count());
    const auto second = instance.allocate(little_slab::count());
    BC_REQUIRE_EQUAL(first, 0u);
    BC_REQUIRE_EQUAL(second, sizeof(uint32_t));

    BC_REQUIRE(instance.put(first, little_slab{ 0x01020304_u32 }));
    BC_REQUIRE(instance.put(second, little_slab{ 0x05060708_u32 }));

    little_slab out{};
    BC_REQUIRE(instance.get(first, out));
    BC_REQUIRE_EQUAL(out.value, 0x01020304_u32);
    BC_REQUIRE(instance.get(second, out));
    BC_REQUIRE_EQUAL(out.value, 0x05060708_u32);
}

// The element is rewritten in place, as the head is not appended.
BC_AUTO_TEST_CASE(headmap__slab_put__rewrite__expected_get)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    slab_headmap instance{ head_store };
    BC_REQUIRE(instance.create());

    const auto link = instance.allocate(little_slab::count());
    BC_REQUIRE(instance.put(link, little_slab{ 0x11223344_u32 }));
    BC_REQUIRE(instance.put(link, little_slab{ 0x55667788_u32 }));
    BC_REQUIRE_EQUAL(instance.head_size(), sizeof(uint32_t));

    little_slab out{};
    BC_REQUIRE(instance.get(link, out));
    BC_REQUIRE_EQUAL(out.value, 0x55667788_u32);
}

BC_AUTO_TEST_CASE(headmap__slab_get__terminal__false)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    slab_headmap instance{ head_store };
    BC_REQUIRE(instance.create());

    little_slab out{};
    BC_REQUIRE(!instance.get(slab_link::terminal, out));
    BC_REQUIRE(!instance.put(slab_link::terminal, little_slab{}));
}

BC_AUTO_TEST_CASE(headmap__slab_get__unallocated__false)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    slab_headmap instance{ head_store };
    BC_REQUIRE(instance.create());

    little_slab out{};
    BC_REQUIRE(!instance.get(0u, out));
    BC_REQUIRE(!instance.put(0u, little_slab{}));
}

BC_AUTO_TEST_CASE(headmap__slab_setup__lifecycle__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    slab_headmap instance{ head_store };
    BC_REQUIRE(instance.create());

    const auto link = instance.allocate(little_slab::count());
    BC_REQUIRE(instance.put(link, little_slab{ 0x11223344_u32 }));
    BC_REQUIRE(instance.close());
    BC_REQUIRE(instance.backup());
    BC_REQUIRE(instance.restore());
    BC_REQUIRE(instance.verify());
    BC_REQUIRE(!instance.get_fault());

    little_slab out{};
    BC_REQUIRE(instance.get(link, out));
    BC_REQUIRE_EQUAL(out.value, 0x11223344_u32);
}

// cell get/set
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(headmap__get__beyond_count__false)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    value_headmap instance{ head_store };
    BC_REQUIRE(instance.create());

    byte_get out{};
    BC_REQUIRE(!instance.get(0u, out));
}

BC_AUTO_TEST_CASE(headmap__set__byte_beyond_count__filled_empty)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    value_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.set(3u, byte_put{ 0x2a }));
    BC_REQUIRE_EQUAL(instance.count(), 4u);
    BC_REQUIRE_EQUAL(head_file, base16_chunk("0000002a"));

    byte_get out{};
    BC_REQUIRE(instance.get(2u, out));
    BC_REQUIRE_EQUAL(out.value, 0x00u);
    BC_REQUIRE(instance.get(3u, out));
    BC_REQUIRE_EQUAL(out.value, 0x2au);
    BC_REQUIRE(!instance.get(4u, out));
}

BC_AUTO_TEST_CASE(headmap__set__terminal__false)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    value_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(!instance.set(value_link::terminal, byte_put{ 0x2a }));
    BC_REQUIRE_EQUAL(instance.count(), 0u);
    BC_REQUIRE(head_file.empty());
}

BC_AUTO_TEST_CASE(headmap__set_indexes__beyond_count__filled_empty)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    value_headmap instance{ head_store };
    BC_REQUIRE(instance.create());

    const std::vector<value_link::integer> indexes{ 3u, 1u };
    BC_REQUIRE(instance.set(indexes, byte_put{ 0x2a }));
    BC_REQUIRE_EQUAL(instance.count(), 4u);
    BC_REQUIRE_EQUAL(head_file, base16_chunk("002a002a"));
}

BC_AUTO_TEST_CASE(headmap__set_indexes__empty__true)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    value_headmap instance{ head_store };
    BC_REQUIRE(instance.create());

    const std::vector<value_link::integer> indexes{};
    BC_REQUIRE(instance.set(indexes, byte_put{ 0x2a }));
    BC_REQUIRE_EQUAL(instance.count(), 0u);
    BC_REQUIRE(head_file.empty());
}

BC_AUTO_TEST_CASE(headmap__set_indexes__terminal__false)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    value_headmap instance{ head_store };
    BC_REQUIRE(instance.create());

    const std::vector<value_link::integer> indexes{ 1u, value_link::terminal };
    BC_REQUIRE(!instance.set(indexes, byte_put{ 0x2a }));
    BC_REQUIRE_EQUAL(instance.count(), 0u);
    BC_REQUIRE(head_file.empty());
}

BC_AUTO_TEST_CASE(headmap__set__byte_overwrite__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    value_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.set(1u, byte_put{ 0x01 }));
    BC_REQUIRE(instance.set(1u, byte_put{ 0x02 }));
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(instance.at(1), 0x02u);
}

BC_AUTO_TEST_CASE(headmap__set__flags__merged)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    value_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.set(2u, flags_put{ 0x01 }));
    BC_REQUIRE(instance.set(2u, flags_put{ 0x04 }));
    BC_REQUIRE_EQUAL(instance.count(), 3u);
    BC_REQUIRE_EQUAL(head_file, base16_chunk("000005"));
    BC_REQUIRE_EQUAL(instance.at(2), 0x05u);
}

BC_AUTO_TEST_CASE(headmap__set__concurrent_flags__all_merged)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    value_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.set(0u, byte_put{}));

    std::vector<size_t> rounds(byte_bits * 1000u);
    std::iota(rounds.begin(), rounds.end(), zero);
    std::for_each(poolstl::execution::par, rounds.cbegin(), rounds.cend(),
        [&](size_t round) NOEXCEPT
        {
            const flags_put flag{ shift_left<uint8_t>(1u, round % byte_bits) };
            instance.set(0u, flag);
        });

    BC_REQUIRE_EQUAL(instance.count(), 1u);
    BC_REQUIRE_EQUAL(instance.at(0), 0xffu);
}

BC_AUTO_TEST_CASE(headmap__set__link_beyond_count__filled_terminal)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    test_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.set(2u, link_put{ 0x1122 }));
    BC_REQUIRE_EQUAL(instance.count(), 3u);
    BC_REQUIRE_EQUAL(head_file, base16_chunk("ffffffff2211"));
    BC_REQUIRE(instance.at(0).is_terminal());
    BC_REQUIRE(instance.at(1).is_terminal());
    BC_REQUIRE_EQUAL(instance.at(2), 0x1122_u16);
}

BC_AUTO_TEST_CASE(headmap__get__unaligned_link__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file, test::terminal_fill };
    unaligned_headmap instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.reserve(1u));
    BC_REQUIRE(instance.push(0x112233_u32));

    small_link_get out{};
    BC_REQUIRE(instance.get(0u, out));
    BC_REQUIRE_EQUAL(out.value, 0x112233_u32);
}

BC_AUTO_TEST_SUITE_END()
