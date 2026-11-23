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
#include "../mocks/blocks.hpp"
#include "../mocks/map_store.hpp"

// these include the slow tests (mmap)

BC_FIXTURE_TEST_SUITE(store_tests, test::directory_setup_fixture)

// open
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__open__transactor_locked__transactor_lock)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    instance.transactor_mutex().lock();
    BC_REQUIRE_EQUAL(instance.open(test::events), error::transactor_lock);
}

// The lock is process-exclusive in linux/macOS, globally in win32.
#if defined(HAVE_WINDOWS)
BC_AUTO_TEST_CASE(store__copen__process_locked__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    interprocess_lock lock{ instance.process_lock_file() };
    BC_REQUIRE(lock.try_lock());
    BC_REQUIRE_EQUAL(instance.open(test::events), error::process_lock);
}
#endif

BC_AUTO_TEST_CASE(store__open__fault_locked__fault_lock)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(test::create(instance.fault_lock_file()));
    BC_REQUIRE_EQUAL(instance.open(test::events), error::fault_lock);
    BC_REQUIRE(!test::exists(instance.flush_lock_file()));
}

BC_AUTO_TEST_CASE(store__open__fault_and_flush_locked__fault_lock)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(test::create(instance.fault_lock_file()));
    BC_REQUIRE(test::create(instance.flush_lock_file()));
    BC_REQUIRE_EQUAL(instance.open(test::events), error::fault_lock);
}

BC_AUTO_TEST_CASE(store__open__fault_lock_set_and_closed__fault_lock)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.is_fault_locked());
    BC_REQUIRE(instance.set_fault_lock());
    BC_REQUIRE(instance.set_fault_lock());
    BC_REQUIRE(instance.is_fault_locked());
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(test::exists(instance.fault_lock_file()));
    BC_REQUIRE(!test::exists(instance.flush_lock_file()));
    BC_REQUIRE_EQUAL(instance.open(test::events), error::fault_lock);
}

BC_AUTO_TEST_CASE(store__open__flush_locked__flush_lock)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(test::create(instance.flush_lock_file()));
    BC_REQUIRE_EQUAL(instance.open(test::events), error::flush_lock);
}

BC_AUTO_TEST_CASE(store__open__process_lock_file__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(test::create(instance.process_lock_file()));
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__default__unlocks_transactor_only)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(test::exists(instance.flush_lock_file()));
    BC_REQUIRE(test::exists(instance.process_lock_file()));
    BC_REQUIRE(instance.transactor_mutex().try_lock());
    instance.transactor_mutex().unlock();
    instance.close(test::events);
}

BC_AUTO_TEST_CASE(store__open__uncreated__open_failure)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    BC_REQUIRE(instance.open(test::events));
}

BC_AUTO_TEST_CASE(store__open__created__success)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__schema_mismatch__schema_version)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(test::create(instance.envelope_head_file(), std::string(16, '')));
    BC_REQUIRE_EQUAL(instance.open(test::events), error::schema_version);
}

BC_AUTO_TEST_CASE(store__open__4_0_31__schema_version)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));

    const auto& image = test::envelope_4_0_31;
    BC_REQUIRE(test::create(instance.envelope_head_file(), image));
    BC_REQUIRE_EQUAL(instance.open(test::events), error::schema_version);
    BC_REQUIRE_EQUAL(test::size(instance.envelope_head_file()), image.size());
}

// migrate
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__migrate__uncreated__failure)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    system::config::version original{};
    BC_REQUIRE(instance.migrate(original, {}, test::events));
}

BC_AUTO_TEST_CASE(store__migrate__compiled__unchanged)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(test::clear(configuration.path / schema::dir::primary));

    system::config::version original{};
    BC_REQUIRE(!instance.migrate(original, {}, test::events));
    BC_REQUIRE(original == envelope::compiled);
    BC_REQUIRE(test::folder(configuration.path / schema::dir::primary));
    BC_REQUIRE(!test::exists(instance.fault_lock_file()));
    BC_REQUIRE(!test::exists(instance.flush_lock_file()));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__migrate__4_0_30__compiled)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));

    auto image = test::envelope_4_0_31;
    image.at(8) = 30;
    BC_REQUIRE(test::create(instance.envelope_head_file(), image));

    const auto pool_head = test::size(instance.pool_head_file());
    const auto wtxid_head = test::size(instance.wtxid_head_file());
    const auto obsolete = configuration.path / "cache_pool_context.data";
    BC_REQUIRE(test::create(instance.pool_head_file(), std::string{ "stale" }));
    BC_REQUIRE(test::create(instance.wtxid_head_file(), std::string{ "stale" }));
    BC_REQUIRE(test::create(obsolete, std::string{ "stale" }));

    database::envelope configured{};
    configured.require_witness = true;
    configured.pool_buckets = 256;
    configured.wtxid_buckets = 512;
    system::config::version original{};
    BC_REQUIRE(!instance.migrate(original, configured, test::events));
    BC_REQUIRE(original == system::config::version(4, 0, 30, 0));
    BC_REQUIRE(!test::exists(obsolete));
    BC_REQUIRE_GT(test::size(instance.pool_head_file()), pool_head);
    BC_REQUIRE_GT(test::size(instance.wtxid_head_file()), wtxid_head);
    BC_REQUIRE_EQUAL(test::size(instance.envelope_head_file()), image.size() + 4u);
    BC_REQUIRE(!test::exists(instance.fault_lock_file()));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE(!instance.close(test::events));

    database::envelope stored{};
    BC_REQUIRE(!instance.read_envelope(stored));
    BC_REQUIRE(stored.schema == envelope::compiled);
    BC_REQUIRE(stored.require_witness);
    BC_REQUIRE_EQUAL(stored.pool_buckets, 256u);
    BC_REQUIRE_EQUAL(stored.wtxid_buckets, 512u);
}

BC_AUTO_TEST_CASE(store__migrate__4_0_31__compiled)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));

    const auto& image = test::envelope_4_0_31;
    BC_REQUIRE(test::create(instance.envelope_head_file(), image));

    database::envelope configured{};
    configured.require_witness = true;
    system::config::version original{};
    BC_REQUIRE(!instance.migrate(original, configured, test::events));
    BC_REQUIRE(original == system::config::version(4, 0, 31, 0));
    BC_REQUIRE_EQUAL(test::size(instance.envelope_head_file()), image.size() + 4u);
    BC_REQUIRE(!test::exists(instance.fault_lock_file()));
    BC_REQUIRE(!test::exists(instance.flush_lock_file()));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE(!instance.close(test::events));

    database::envelope stored{};
    BC_REQUIRE(!instance.read_envelope(stored));
    BC_REQUIRE(stored.schema == envelope::compiled);
    BC_REQUIRE(stored.network == system::chain::selection::mainnet);
    BC_REQUIRE(stored.require_witness);
    BC_REQUIRE(stored.provide_filters);
    BC_REQUIRE_EQUAL(stored.silent_start_height, 42u);
}

BC_AUTO_TEST_CASE(store__migrate__4_0_33__pool_top_terminal_activation_kept)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));

    // Last activation link is the last three bytes.
    database::envelope configured{};
    auto image = test::envelope_4_0_31;
    BC_REQUIRE(envelope::migrate_step(image, configured));
    BC_REQUIRE(envelope::migrate_step(image, configured));
    image.at(image.size() - 3u) = 42;
    image.at(image.size() - 2u) = 0;
    image.at(image.size() - 1u) = 0;
    BC_REQUIRE(test::create(instance.envelope_head_file(), image));

    system::config::version original{};
    BC_REQUIRE(!instance.migrate(original, configured, test::events));
    BC_REQUIRE(original == system::config::version(4, 0, 33, 0));
    BC_REQUIRE_EQUAL(test::size(instance.envelope_head_file()), image.size() + 3u);
    BC_REQUIRE(!test::exists(instance.fault_lock_file()));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(instance.get_pool_top().value, schema::header::link::terminal);
    BC_REQUIRE(!instance.close(test::events));

    database::envelope stored{};
    BC_REQUIRE(!instance.read_envelope(stored));
    BC_REQUIRE(stored.schema == envelope::compiled);
    BC_REQUIRE_EQUAL(stored.pool_top, schema::header::link::terminal);
    BC_REQUIRE_EQUAL(stored.activations.back().link, 42u);
}

// Archive a pruned block and clear its flags, as stored before 4.0.33.
static bool set_unflagged_pruned_block(test::map_store& store) NOEXCEPT
{
    database::query<test::map_store> query{ store };
    if (!query.initialize(test::genesis) ||
        !query.set(test::block1.header(), test::context, {}, false) ||
        !query.set(test::block1, false, false, true))
        return false;

    table::transaction::record record{};
    if (!store.tx.get(1, record))
        return false;

    record.pruned = false;
    if (!store.tx.set(1, store.tx.get_key(1), record))
        return false;

    table::txs::slab slab{};
    const auto link = store.txs.at(1);
    if (!store.txs.get(link, slab))
        return false;

    slab.pruned = false;
    return store.txs.set(link, slab) && !query.is_pruned(tx_link{ 1 }) &&
        !query.is_pruned(header_link{ 1 });
}

// Allocate a (zero) row without setting or committing it.
static tx_link set_uncommitted_row(test::map_store& store) NOEXCEPT
{
    return store.tx.allocate(1);
}

BC_AUTO_TEST_CASE(store__migrate__4_0_31_uncommitted__skipped)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(set_unflagged_pruned_block(instance));
    const auto link = set_uncommitted_row(instance);
    BC_REQUIRE(!link.is_terminal());
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(test::create(instance.envelope_head_file(), test::envelope_4_0_31));

    database::envelope configured{};
    configured.require_witness = true;
    system::config::version original{};
    BC_REQUIRE(!instance.migrate(original, configured, test::events));
    BC_REQUIRE(!test::exists(instance.fault_lock_file()));
    BC_REQUIRE(!instance.open(test::events));

    table::transaction::record record{};
    BC_REQUIRE(instance.tx.get(link, record));
    BC_REQUIRE(record == table::transaction::record{});
    BC_REQUIRE(database::query<test::map_store>{ instance }.is_pruned(tx_link{ 1 }));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__migrate__4_0_31_pruned__flagged)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(set_unflagged_pruned_block(instance));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(test::create(instance.envelope_head_file(), test::envelope_4_0_31));

    database::envelope configured{};
    configured.require_witness = true;
    system::config::version original{};
    BC_REQUIRE(!instance.migrate(original, configured, test::events));
    BC_REQUIRE(!test::exists(instance.fault_lock_file()));
    BC_REQUIRE(!instance.open(test::events));

    const database::query<test::map_store> query{ instance };
    BC_REQUIRE(query.is_pruned(tx_link{ 1 }));
    BC_REQUIRE(query.is_pruned(header_link{ 1 }));
    BC_REQUIRE(!query.is_pruned(tx_link{ 0 }));
    BC_REQUIRE(!query.is_pruned(header_link{ 0 }));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__migrate__4_0_31__snapshots_purged)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(test::create(instance.envelope_head_file(), test::envelope_4_0_31));
    BC_REQUIRE(test::clear(configuration.path / schema::dir::primary));
    BC_REQUIRE(test::clear(configuration.path / schema::dir::secondary));
    BC_REQUIRE(test::create(configuration.path / schema::dir::primary / "file"));

    system::config::version original{};
    BC_REQUIRE(!instance.migrate(original, {}, test::events));
    BC_REQUIRE(!test::folder(configuration.path / schema::dir::primary));
    BC_REQUIRE(!test::folder(configuration.path / schema::dir::secondary));
}

BC_AUTO_TEST_CASE(store__migrate__4_0_31_truncated__verify_table_unlocked)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));

    const auto& image = test::envelope_4_0_31;
    const system::data_chunk truncated(image.begin(), std::next(image.begin(), 16));
    BC_REQUIRE(test::create(instance.envelope_head_file(), truncated));

    system::config::version original{};
    BC_REQUIRE_EQUAL(instance.migrate(original, {}, test::events), error::verify_table);
    BC_REQUIRE(original == system::config::version(4, 0, 31, 0));
    BC_REQUIRE(!test::exists(instance.fault_lock_file()));
    BC_REQUIRE(!test::exists(instance.flush_lock_file()));
}

BC_AUTO_TEST_CASE(store__migrate__4_0_31_truncated__snapshots_retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));

    const auto& image = test::envelope_4_0_31;
    const system::data_chunk truncated(image.begin(), std::next(image.begin(), 16));
    BC_REQUIRE(test::create(instance.envelope_head_file(), truncated));
    BC_REQUIRE(test::clear(configuration.path / schema::dir::primary));
    BC_REQUIRE(test::create(configuration.path / schema::dir::primary / "file"));

    system::config::version original{};
    BC_REQUIRE_EQUAL(instance.migrate(original, {}, test::events), error::verify_table);
    BC_REQUIRE(test::exists(configuration.path / schema::dir::primary / "file"));
}

BC_AUTO_TEST_CASE(store__migrate__premigratable__schema_version)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));

    auto image = test::envelope_4_0_31;
    const auto prior = sub1(envelope::migratable.segments().at(2));
    const auto bytes = system::to_little_endian(prior);
    std::copy(bytes.begin(), bytes.end(), std::next(image.begin(), 8));
    BC_REQUIRE(test::create(instance.envelope_head_file(), image));

    system::config::version original{};
    BC_REQUIRE_EQUAL(instance.migrate(original, {}, test::events), error::schema_version);
    BC_REQUIRE(original == system::config::version(4, 0, prior, 0));
    BC_REQUIRE(!test::exists(instance.fault_lock_file()));
    BC_REQUIRE(!test::exists(instance.flush_lock_file()));
    BC_REQUIRE_EQUAL(test::size(instance.envelope_head_file()), image.size());
}

BC_AUTO_TEST_CASE(store__migrate__future__schema_version)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(test::create(instance.envelope_head_file(), std::string(16, '\x01')));

    system::config::version original{};
    const system::config::version expected{ 0x01010101, 0x01010101, 0x01010101, 0x01010101 };
    BC_REQUIRE_EQUAL(instance.migrate(original, {}, test::events), error::schema_version);
    BC_REQUIRE(original == expected);
    BC_REQUIRE(!test::exists(instance.fault_lock_file()));
    BC_REQUIRE(!test::exists(instance.flush_lock_file()));
}

// read_envelope
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(store__read_envelope__uncreated__failure)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    database::envelope stored{};
    test::map_store instance{ configuration };
    BC_REQUIRE(instance.read_envelope(stored));
}

BC_AUTO_TEST_CASE(store__read_envelope__created__stored)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    configuration.envelope.network = system::chain::selection::testnet4;
    database::envelope stored{};
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.read_envelope(stored));
    BC_REQUIRE(stored.schema == envelope::compiled);
    BC_REQUIRE(stored.network == system::chain::selection::testnet4);
}

BC_AUTO_TEST_CASE(store__read_envelope__4_0_31__migrated_unchanged)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));

    const auto& image = test::envelope_4_0_31;
    BC_REQUIRE(test::create(instance.envelope_head_file(), image));

    database::envelope stored{};
    stored.require_witness = true;
    BC_REQUIRE(!instance.read_envelope(stored));
    BC_REQUIRE(stored.schema == system::config::version(4, 0, 31, 0));
    BC_REQUIRE(stored.network == system::chain::selection::mainnet);
    BC_REQUIRE(stored.require_witness);
    BC_REQUIRE_EQUAL(stored.silent_start_height, 42u);
    BC_REQUIRE_EQUAL(test::size(instance.envelope_head_file()), image.size());

    database::envelope reread{};
    BC_REQUIRE(!instance.read_envelope(reread));
    BC_REQUIRE(reread.schema == system::config::version(4, 0, 31, 0));
}

BC_AUTO_TEST_CASE(store__read_envelope__schema_mismatch__schema_version)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    database::envelope stored{};
    const system::config::version expected{ 0x01010101, 0x01010101, 0x01010101, 0x01010101 };
    test::map_store instance{ configuration };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(test::create(instance.envelope_head_file(), std::string(16, '\x01')));
    BC_REQUIRE_EQUAL(instance.read_envelope(stored), error::schema_version);
    BC_REQUIRE(stored.schema == expected);
}

constexpr database::context validated_context
{
    system::chain::flags::bip113_rule, 8, 9
};

// compaction
// ----------------------------------------------------------------------------

using spend_t = system::chain::transaction;

static spend_t spend(const system::hash_digest& hash, uint64_t value) NOEXCEPT
{
    const system::chain::point point{ hash, 0 };
    const system::chain::input input{ point, system::chain::script{}, max_uint32 };
    const system::chain::output output{ value, system::chain::script{} };
    return { 1u, system::chain::inputs{ input }, system::chain::outputs{ output }, 0u };
}

static const system::hash_digest& genesis_coinbase() NOEXCEPT
{
    const auto& coinbase = *test::genesis.transactions_ptr()->front();
    static const auto hash = coinbase.hash(false);
    return hash;
}

// Archive the tx and pool it under the validated context.
static bool pool(query<store<database::mmap>>& query_, const spend_t& tx) NOEXCEPT
{
    const auto stored = query_.set(tx);
    const auto link = query_.to_tx(tx.hash(false));
    return stored && query_.set_pooled(link, tx, validated_context, 0);
}

// Confirm block of coinbase and tx above genesis (pooled tx is associated by
// its pooled link, as store is pooling).
static bool confirm(query<store<database::mmap>>& query_, const spend_t& tx) NOEXCEPT
{
    const system::chain::point null{ system::null_hash, system::chain::point::null_index };
    const system::chain::input input{ null, system::chain::script{}, max_uint32 };
    const system::chain::output output{ 42u, system::chain::script{} };
    const spend_t coinbase{ 1u, system::chain::inputs{ input }, system::chain::outputs{ output }, 0u };
    const system::chain::header header{ 1u, test::genesis.hash(), system::null_hash, 1u, 2u, 3u };
    const system::chain::block block{ header, { coinbase, tx } };
    const auto stored = query_.set(block, context{ 0, 1, 0 }, {}, false, true);
    const auto link = query_.to_header(block.hash());
    const auto candidate = stored && query_.push_candidate(link);
    return candidate && query_.push_confirmed(link, true);
}

// Each of first count inputs returns to itself along its ring of rivals,
// after visiting every other of them.
static bool is_ring(const store<database::mmap>& instance, size_t count) NOEXCEPT
{
    for (size_t start{}; start < count; ++start)
    {
        auto at = start;
        for (size_t step{}; step < count; ++step)
            at = instance.spends_index.rivals.at(at);

        if (at != start)
            return false;
    }

    return true;
}

static code pooled(query<store<database::mmap>>& query_, const spend_t& tx) NOEXCEPT
{
    pooled_tx out{};
    out.prevouts.resize(one);
    const auto link = query_.to_tx(tx.hash(false));
    return query_.get_pooled(out, link, validated_context);
}

BC_AUTO_TEST_CASE(store__open__unconfirmed_pooled__retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto unconfirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, unconfirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));

    pooled_tx out{};
    out.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query_.get_pooled(out, query_.to_tx(unconfirmed.hash(false)), validated_context), error::success);
    BC_REQUIRE_EQUAL(out.prevouts.front().parent, 0u);
    BC_REQUIRE(out.prevouts.front().coinbase);
    BC_REQUIRE_EQUAL(query_.spends_records(), one);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__confirmed_pooled__dropped)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto confirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, confirmed), error::unvalidated);
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);
    BC_REQUIRE_EQUAL(query_.spends_records(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__conflicting_pooled__dropped)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto conflict = spend(genesis_coinbase(), 2u);
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(confirm(query_, spend(genesis_coinbase(), 1u)));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, conflict), error::unvalidated);
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__descendant_of_conflicting__dropped)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto conflict = spend(genesis_coinbase(), 2u);
    const auto descendant = spend(conflict.hash(false), 1u);
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(pool(query_, descendant));
    BC_REQUIRE(confirm(query_, spend(genesis_coinbase(), 1u)));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, conflict), error::unvalidated);
    BC_REQUIRE_EQUAL(pooled(query_, descendant), error::unvalidated);
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__descendant_of_retained__retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto parent = spend(genesis_coinbase(), 2u);
    const auto child = spend(parent.hash(false), 1u);
    BC_REQUIRE(pool(query_, parent));
    BC_REQUIRE(pool(query_, child));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, parent), error::success);

    pooled_tx out{};
    out.prevouts.resize(one);
    BC_REQUIRE_EQUAL(query_.get_pooled(out, query_.to_tx(child.hash(false)), validated_context), error::success);
    BC_REQUIRE_EQUAL(out.prevouts.front().parent, query_.to_tx(parent.hash(false)));
    BC_REQUIRE(!out.prevouts.front().coinbase);
    BC_REQUIRE_EQUAL(query_.spends_records(), two);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__retained_and_dropped__only_retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto conflict = spend(genesis_coinbase(), 2u);
    const auto retained = spend(spend(genesis_coinbase(), 1u).hash(false), 3u);
    const auto confirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(pool(query_, retained));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, conflict), error::unvalidated);
    BC_REQUIRE_EQUAL(pooled(query_, confirmed), error::unvalidated);
    BC_REQUIRE_EQUAL(pooled(query_, retained), error::success);
    BC_REQUIRE_EQUAL(query_.spends_records(), one);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__child_pooled_before_parent__retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto parent = spend(genesis_coinbase(), 2u);
    const auto child = spend(parent.hash(false), 1u);
    BC_REQUIRE(query_.set(parent));
    BC_REQUIRE(pool(query_, child));
    BC_REQUIRE(query_.set_pooled(query_.to_tx(parent.hash(false)), parent, validated_context, 0));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, parent), error::success);
    BC_REQUIRE_EQUAL(pooled(query_, child), error::success);
    BC_REQUIRE_EQUAL(query_.spends_records(), two);
    BC_REQUIRE_EQUAL(query_.wtxid_records(), two);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__child_of_unpooled_parent__dropped)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto parent = spend(genesis_coinbase(), 2u);
    const auto child = spend(parent.hash(false), 1u);
    BC_REQUIRE(query_.set(parent));
    BC_REQUIRE(pool(query_, child));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, child), error::unvalidated);
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);
    BC_REQUIRE_EQUAL(query_.wtxid_records(), zero);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__retained__found_by_witness_hash)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto conflict = spend(genesis_coinbase(), 2u);
    const auto retained = spend(spend(genesis_coinbase(), 1u).hash(false), 3u);
    const auto confirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(pool(query_, retained));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(query_.wtxid_records(), one);
    BC_REQUIRE_EQUAL(query_.to_witness_tx(retained.hash(true)), query_.to_tx(retained.hash(false)));
    BC_REQUIRE(!query_.is_pooled(query_.to_tx(conflict.hash(false))));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__retained_around_dropped__moved_with_spends)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto confirmed = spend(genesis_coinbase(), 1u);
    const auto parent = spend(confirmed.hash(false), 3u);
    const auto conflict = spend(genesis_coinbase(), 2u);
    const auto child = spend(parent.hash(false), 4u);
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(pool(query_, parent));
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(pool(query_, child));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));

    pooled_tx out{};
    out.prevouts.resize(one);
    BC_REQUIRE_EQUAL(pooled(query_, confirmed), error::unvalidated);
    BC_REQUIRE_EQUAL(pooled(query_, conflict), error::unvalidated);
    BC_REQUIRE_EQUAL(pooled(query_, parent), error::success);
    BC_REQUIRE_EQUAL(query_.get_pooled(out, query_.to_tx(child.hash(false)), validated_context), error::success);
    BC_REQUIRE_EQUAL(out.prevouts.front().parent, query_.to_tx(parent.hash(false)));
    BC_REQUIRE_EQUAL(query_.spends_records(), two);
    BC_REQUIRE_EQUAL(query_.wtxid_records(), two);
    BC_REQUIRE_EQUAL(query_.to_witness_tx(parent.hash(true)), query_.to_tx(parent.hash(false)));
    BC_REQUIRE_EQUAL(query_.to_witness_tx(child.hash(true)), query_.to_tx(child.hash(false)));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__retained_and_dropped__counts_recorded)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto conflict = spend(genesis_coinbase(), 2u);
    const auto retained = spend(spend(genesis_coinbase(), 1u).hash(false), 3u);
    const auto confirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(pool(query_, retained));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(query_.pool_records(), one);
    BC_REQUIRE(instance.pool.verify());
    BC_REQUIRE(instance.spends.verify());
    BC_REQUIRE(instance.wtxid.verify());
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__all_dropped__counts_recorded)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto confirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(query_.pool_records(), zero);
    BC_REQUIRE(instance.pool.verify());
    BC_REQUIRE(instance.spends.verify());
    BC_REQUIRE(instance.wtxid.verify());
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__retained_and_dropped__retained_present)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto conflict = spend(genesis_coinbase(), 2u);
    const auto retained = spend(spend(genesis_coinbase(), 1u).hash(false), 3u);
    const auto confirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(pool(query_, retained));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(query_.pool_records(), one);
    BC_REQUIRE(system::pool::algorithm::is_present(instance.pool_index.cells.at(zero)));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__retained_rivals__closed_ring)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto confirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(pool(query_, spend(confirmed.hash(false), 5u)));
    BC_REQUIRE(pool(query_, spend(confirmed.hash(false), 4u)));
    BC_REQUIRE(pool(query_, spend(confirmed.hash(false), 3u)));
    BC_REQUIRE(pool(query_, spend(confirmed.hash(false), 2u)));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(query_.pool_records(), 4u);
    BC_REQUIRE(is_ring(instance, 4u));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__compacted_reopened__retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto conflict = spend(genesis_coinbase(), 2u);
    const auto retained = spend(spend(genesis_coinbase(), 1u).hash(false), 3u);
    const auto confirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, conflict));
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(pool(query_, retained));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, retained), error::success);
    BC_REQUIRE_EQUAL(query_.spends_records(), one);
    BC_REQUIRE_EQUAL(query_.wtxid_records(), one);
    BC_REQUIRE_EQUAL(query_.to_witness_tx(retained.hash(true)), query_.to_tx(retained.hash(false)));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__compacted__pooled_after)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto confirmed = spend(genesis_coinbase(), 1u);
    const auto retained = spend(confirmed.hash(false), 3u);
    const auto added = spend(retained.hash(false), 4u);
    BC_REQUIRE(pool(query_, confirmed));
    BC_REQUIRE(pool(query_, retained));
    BC_REQUIRE(confirm(query_, confirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE(pool(query_, added));

    pooled_tx out{};
    out.prevouts.resize(one);
    BC_REQUIRE_EQUAL(pooled(query_, retained), error::success);
    BC_REQUIRE_EQUAL(query_.get_pooled(out, query_.to_tx(added.hash(false)), validated_context), error::success);
    BC_REQUIRE_EQUAL(out.prevouts.front().parent, query_.to_tx(retained.hash(false)));
    BC_REQUIRE_EQUAL(query_.spends_records(), two);
    BC_REQUIRE_EQUAL(query_.wtxid_records(), two);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__pool_top__retained)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    BC_REQUIRE(confirm(query_, spend(genesis_coinbase(), 1u)));
    BC_REQUIRE(query_.set_pool_block(query_.to_confirmed(1), true));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(query_.get_pool_top().value, 1u);
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__restore__pool_top__terminal)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    BC_REQUIRE(query_.set_pool_top(query_.to_confirmed(0)));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE(!query_.get_pool_top().is_terminal());
    BC_REQUIRE(!instance.snapshot(test::events));
    BC_REQUIRE(!instance.close(test::events));

    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(!instance.restore(test::events));
    BC_REQUIRE(query_.get_pool_top().is_terminal());
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE(query_.get_pool_top().is_terminal());
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_CASE(store__open__retained_snapshot_faulted_restore__pool_empty)
{
    settings configuration{ test::database_settings() };
    configuration.path = TEST_DIRECTORY;
    store<database::mmap> instance{ configuration };
    query<store<database::mmap>> query_{ instance };
    BC_REQUIRE(!instance.create(test::events));
    BC_REQUIRE(query_.initialize(test::genesis));
    instance.set_pooling();

    const auto unconfirmed = spend(genesis_coinbase(), 1u);
    BC_REQUIRE(pool(query_, unconfirmed));
    BC_REQUIRE(!instance.close(test::events));
    BC_REQUIRE(!instance.open(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, unconfirmed), error::success);
    BC_REQUIRE(!instance.snapshot(test::events));
    BC_REQUIRE(!instance.close(test::events));

    // A restore drops the pool.
    BC_REQUIRE(test::create(test::flush_lock_file(configuration.path)));
    BC_REQUIRE(!instance.restore(test::events));
    BC_REQUIRE_EQUAL(pooled(query_, unconfirmed), error::unvalidated);
    BC_REQUIRE_EQUAL(query_.pool_body_size(), zero);
    BC_REQUIRE_EQUAL(query_.wtxid_body_size(), zero);
    BC_REQUIRE(!query_.is_pooled(query_.to_tx(unconfirmed.hash(false))));
    BC_REQUIRE(!instance.close(test::events));
}

BC_AUTO_TEST_SUITE_END()
