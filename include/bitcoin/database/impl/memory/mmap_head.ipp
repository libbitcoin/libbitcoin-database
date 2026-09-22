/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_HEAD_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_HEAD_IPP

#include <mutex>
#include <shared_mutex>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mstage.hpp>
#include <bitcoin/database/memory/release.hpp>
#include <bitcoin/database/memory/utilities.hpp>

namespace libbitcoin {
namespace database {

// Write declaration and marking (managed head instances). Marks drive the
// dirty transfer, declarations restore released pages ahead of the write.
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::prepare(size_t STAGING_ONLY(offset),
    size_t STAGING_ONLY(size)) NOEXCEPT
{
#if defined(MANAGE_STAGING)
    if (is_zero(size) || !managed_)
        return;

    // Count the writer before loading released below (sequentially
    // consistent), pairing with the release protocol: release either observes
    // the count (and aborts) or this writer observes released (and restores).
    // Intent bits age (hot sampling), so they cannot protect a write held
    // in flight across passes; the count persists until mark.
    // A settle transition pairs the same way, but the writer waits UNCOUNTED
    // (it retires its count and does not retake one until the transition
    // clears), so the count drains monotonically and the transition is
    // guaranteed to observe zero rather than merely likely to.
    auto& writers = writer_slot_();
    for (;;)
    {
        while (transition_.load())
            std::this_thread::yield();

        writers.fetch_add(one);
        if (!transition_.load())
            break;

        writers.fetch_sub(one);
    }

    // A settled head writes through its mapping.
    if (shared_.load())
        return;

    if (!engaged_.load(relaxed) && !lazy_.load(relaxed))
        return;

    // Declare intent before the write (sequentially consistent, pairing with
    // the release protocol), then restore any released page in the range.
    auto restore = false;
    auto page = offset / page_;
    const auto end = (offset + sub1(size)) / page_;
    while ((page <= end) && ((page / page_bound) < words_))
    {
        const auto word = page / page_bound;
        const auto flag = system::bit_right<uint64_t>(page % page_bound);
        intent_[word].fetch_or(flag);
        restore |= !is_zero(system::bit_and(released_[word].load(), flag));
        ++page;
    }

    if (restore)
        restore_(offset, size);
#endif
}

TEMPLATE
void CLASS::mark(size_t STAGING_ONLY(offset),
    size_t STAGING_ONLY(size)) NOEXCEPT
{
#if defined(MANAGE_STAGING)
    if (is_zero(size) || !managed_)
        return;

    // Marks follow content writes; transfer clears before reading, so pages
    // remarked during a transfer are simply rewritten by the next pass. A
    // settled head writes through its mapping, so its marks count only (the
    // worker reads the rate); no transition intervenes (the writer is
    // counted), so the path matches prepare.
    if (shared_.load())
        marks_.fetch_add(one, relaxed);
    else
        remark_(offset, size);

    // Uncount the writer after its marks (sequentially consistent), so a
    // release pass loading a drained count observes the dirty bits. Only
    // prepare() counts, so only mark() may uncount (transfer failure restores
    // marks by remark_, as an unpaired uncount here corrupts the count).
    writer_slot_().fetch_sub(one);
#endif
}

TEMPLATE
void CLASS::current(bool STAGING_ONLY(state)) NOEXCEPT
{
#if defined(MANAGE_STAGING)
    current_.store(state);
#endif
}

#if defined(MANAGE_STAGING)

// managed head installation, not thread safe.
// ----------------------------------------------------------------------------
// private

// Head-creation fill: the fill is file content, written sequentially to the
// file (page cache, no mapping involvement) and installed released, so
// creation contributes no memory residency (an in-memory backfill of the
// head set is the store's largest allocation transient).
TEMPLATE
size_t CLASS::allocate_filled_(size_t count, uint8_t backfill) NOEXCEPT
{
    BC_ASSERT(managed_ && !shared_.load());
    std::unique_lock field_lock(field_mutex_);
    std::unique_lock map_lock(remap_mutex_);

    using namespace system;
    const auto start = logical_.load();
    if (!loaded_.load() || fault_.load() || is_add_overflow(start, count))
        return storage::eof;

    // Provision the file physically (disk full detected here).
    const auto end = start + count;
    if (!resize_<zero>(end))
        return storage::eof;

    // Write the fill.
    const auto from = to_width<zero>(start);
    const auto to = to_width<zero>(end);
    std::vector<uint8_t> chunk(std::min(to - from, release_chunk), backfill);
    for (auto at = from; at < to; )
    {
        const auto size = std::min(to - at, chunk.size());
        if (!pwrite_all(opened_[zero], chunk.data(), size, at))
        {
            set_first_code(error::fsync_failure);
            return storage::eof;
        }

        at += size;
    }

    // The fill bounds capacity (installation populates only the fill).
    BC_ASSERT(capacity_.load() <= end);
    logical_.store(end);
    file_.store(std::max(file_.load(), end));
    capacity_.store(end);
    if (!lazy_install_())
        return storage::eof;

    check_invariants_();
    return start;
}

// Install the managed head lazily over its current logical span: a released
// (read-only file) full-page prefix restored to anonymous per segment on
// first write, an anonymous populated tail, and clean page tracking sized to
// a replacement reservation. Caller holds exclusive remap (or is loading).
TEMPLATE
bool CLASS::lazy_install_() NOEXCEPT
{
    using namespace system;
    const auto rows = logical_.load();
    const auto logical = to_width<zero>(rows);

    // Replace any standing reservation (sized as stage_).
    if (!is_null(memory_map_[zero]))
        mmap_unreserve(memory_map_[zero], reserved_[zero]);

    const auto reserved = page_ceiling(to_width<zero>(
        to_reservation(to_provision())));
    const auto base = mmap_reserve(reserved);
    if (base == MAP_FAILED)
    {
        set_first_code(error::mmap_failure);
        return false;
    }

    memory_map_[zero] = pointer_cast<uint8_t>(base);
    reserved_[zero] = reserved;

    // Rebuild page tracking for the new reservation (value-initialized).
    const auto pages = ceilinged_divide(reserved, page_);
    words_ = ceilinged_divide(pages, page_bound);
    dirty_ = std::make_unique<dirty_bitmaps>(words_);
    intent_ = std::make_unique<dirty_bitmaps>(words_);
    released_ = std::make_unique<dirty_bitmaps>(words_);
    sweep_ = std::make_unique<uint64_t[]>(words_);
    writers_reset_();

    // Released file prefix (full pages below logical).
    const auto floor = page_floor(logical);
    if (is_nonzero(floor) && (mmap_settle(memory_map_[zero], floor,
        opened_[zero], zero) == fail))
    {
        set_first_code(error::mmap_failure);
        return false;
    }

    // Commit the remainder to the commitment target and populate the tail.
    const auto target = std::max(page_ceiling(to_width<zero>(
        to_commitment())), page_ceiling(logical));
    if ((target > floor) && (mmap_commit(std::next(memory_map_[zero], floor),
        target - floor, headroom_) == fail))
    {
        set_first_code(error::mmap_failure);
        return false;
    }

    if ((logical > floor) && !pread_all(opened_[zero],
        std::next(memory_map_[zero], floor), logical - floor, floor))
    {
        set_first_code(error::fsync_failure);
        return false;
    }

    if (target > floor)
        mmap_wire(std::next(memory_map_[zero], floor), target - floor);

    declare_released_();

    // Attribute the anonymous span for diagnostics (smaps decomposition).
    mmap_name(std::next(memory_map_[zero], floor), reserved - floor,
        filenames_.front().filename().string().c_str());

    return true;
}

// dirty page transfer, lock-free with writers.
// ----------------------------------------------------------------------------
// private

// Dirty marking without the writer uncount (no paired prepare), for internal
// mark restoration (mark() pairs with prepare() and uncounts its writer).
TEMPLATE
void CLASS::remark_(size_t offset, size_t size) NOEXCEPT
{
    auto page = offset / page_;
    const auto end = (offset + sub1(size)) / page_;
    while ((page <= end) && ((page / page_bound) < words_))
    {
        const auto bit = system::bit_right<uint64_t>(page % page_bound);
        dirty_[page++ / page_bound].fetch_or(bit, relaxed);
    }

    marks_.fetch_add(one, relaxed);
}

// Transfer dirty pages within [0, bytes), clearing marks before content
// reads so that concurrently remarked pages transfer on the next pass.
// Marks beyond bytes are retained (backfill above logical transfers when
// logical grows over it). Adjacent dirty pages coalesce into single writes.
TEMPLATE
template <size_t Column>
bool CLASS::transfer_(size_t bytes) NOEXCEPT
{
    using namespace system;
    if (is_zero(bytes))
        return true;

    // One pass at a time: concurrent passes split the claimed dirty set.
    std::unique_lock transfer_lock(transfer_mutex_);

    // Untracked (multi-column unstaged) instances transfer in full.
    if (!managed_)
        return pwrite_all(opened_[Column], memory_map_[Column], bytes, zero);

    size_t from{};
    size_t to{};
    const auto write = [&]() NOEXCEPT
    {
        return (from >= to) || pwrite_all(opened_[Column],
            std::next(memory_map_[Column], from), to - from, from);
    };

    const auto pages = ceilinged_divide(bytes, page_);
    const auto bound = std::min(words_, ceilinged_divide(pages, page_bound));
    for (size_t word{}; word < bound; ++word)
    {
        auto bits = dirty_[word].exchange(zero, relaxed);

        // Retain marks at and above the page bound (boundary word only).
        const auto first = word * page_bound;
        if (pages < (first + page_bound))
        {
            const auto keep = mask_right<uint64_t>(pages - first);
            dirty_[word].fetch_or(bit_and(bits, keep), relaxed);
            bits = bit_and(bits, bit_not(keep));
        }

        for (size_t bit{}; !is_zero(bits) && (bit < page_bound); ++bit)
        {
            if (!get_right(bits, bit))
                continue;

            bits = set_right(bits, bit, false);
            const auto start = (first + bit) * page_;
            const auto end = std::min(start + page_, bytes);
            if (start == to)
            {
                to = end;
                continue;
            }

            if (!write())
            {
                // Restore claimed marks (the failed range, the page that
                // ended it, and this word's unwritten remainder) so failure
                // is retryable, not lossy.
                remark_(from, to - from);
                remark_(start, end - start);
                dirty_[word].fetch_or(bits, relaxed);
                return false;
            }

            from = start;
            to = end;
        }
    }

    if (!write())
    {
        remark_(from, to - from);
        return false;
    }

    return true;
}

// head scheduler (worker thread).
// ----------------------------------------------------------------------------
// private

// Idle transfer for unstaged (rewrite-in-place head) instances: sustained
// mark stillness drains dirty pages (the lazy writer) so a quiescent map
// converges to persisted and snapshot/close transfer approximately nothing.
// Requires no write exclusion: marks follow writes and transfer clears
// before reading, so racing writes remark and transfer on the next pass
// (torn disk pages are unreachable, as live heads are only trusted
// following a clean close).
//
// Memory scarcity additionally pages the head to its own file (the windows
// model): transfer, then release cold clean pages to read-only file mappings
// (reclaimable cache), restored to anonymous by prepare() before any write.
// Release waits one pass after engagement so all writers declare intent.
TEMPLATE
void CLASS::head_run_() NOEXCEPT
{
    const auto scarce = system_memory() / evict_factor;

    // Ticks without a mark before idle draining.
    auto mark = marks_.load();
    auto transferred = mark;
    size_t still{};
    size_t hot{};
    size_t touched{};

    while (tick_())
    {
        const auto top = marks_.load();
        const auto writes = top - mark;
        still = (top == mark) ? std::min(add1(still), idle_seconds) : zero;
        mark = top;

        // A settled head under sustained writes reinstalls lazily.
        if (shared_.load())
        {
            hot = (writes >= unshare_writes) ? add1(hot) : zero;
            if (hot >= unshare_seconds)
            {
                unshare_();
                transferred = top;
                hot = zero;
            }

            continue;
        }

        // A drained head settles to its file mapping while the store is
        // current (settled pages are droppable, anonymous pages are not).
        if (current_.load() && (transferred == top) && share_(transferred))
            continue;

        // Touch pass: assert working-set residency at a bounded rate.
        // Hash-uniform probing is per-page sparse in every phase, so the
        // kernel ages head pages cold and swaps them under cache pressure,
        // though the set is the process working set (and head misses are
        // unshieldable serial faults on the probe path). A read sets the
        // page-table accessed bit without dirtying the page; volatile
        // prevents elision. The unprivileged equivalent of mlock, and soft:
        // under true extremity the kernel can still take the pages. The
        // per-tick budget revisits every page each touch_seconds regardless
        // of instance size (a whole-instance lap gave the largest head a
        // proportionally longer revisit, which lost the aging race first).
        // Residency-guarded so the touch never faults: a swapped page that
        // nothing probes rests in swap, one the workload needs returns by
        // its own fault and is defended thereafter.
        // Residency is only contested under scarcity, so at plenty the tick
        // costs a counter test (the aging race has no other runner).
        if (system_available() < (system_memory() / sweep_factor))
        {
            using namespace system;
            std::shared_lock touch_lock(remap_mutex_);
            if (loaded_.load() && !fault_.load())
            {
                const auto pages = to_width<zero>(logical_.load()) / page_;
                auto budget = ceilinged_divide(pages, touch_seconds);
#if !defined(HAVE_APPLE)
                // Probe buffer and map alias, declared with the touch they
                // serve (excluded on darwin below), as they are otherwise
                // unused there.
                unsigned char resident[touch_span];
                const volatile auto* map = memory_map_[zero];
#endif
                while (!is_zero(pages) && !is_zero(budget))
                {
                    if (touched >= pages)
                        touched = zero;

                    const auto count = std::min(
                        { touch_span, pages - touched, budget });

#if !defined(HAVE_APPLE)
                    // Darwin excluded: mincore hides compressed pages from
                    // the guard, and unguarded touching measured as pure
                    // decompression churn; release is the darwin mechanism.
                    const auto at = touched * page_;
                    if (mmap_resident(std::next(memory_map_[zero], at),
                        count * page_, resident) == 0)
                        for (size_t page{}; page < count; ++page)
                            if (is_odd(resident[page]))
                                (void)map[at + page * page_];
#endif

                    touched += count;
                    budget = floored_subtract(budget, count);
                }
            }
        }

        // Available includes reclaimable file cache, which a loaded store
        // keeps large while the kernel swaps cold anonymous pages, so free
        // exhaustion also signals scarcity (anon is being displaced).
        // A sync writes every head hot (a converted run restores at the next
        // burst), so release engages only while the store is current.
        const auto scarcity = head_release && managed_ && current_.load() &&
            ((system_available() < scarce) || (system_free() < scarce));

        // Once engaged, a quiet instance converts independent of momentary
        // scarcity: the signal clears as swap absorbs the hot set, but the
        // swapped pages remain anonymous, so reads fault them back one at a
        // time (a serial swap-in per probe). Conversion instead settles
        // clean pages without read-back (the file holds their content),
        // freeing swap and routing reads through the file mapping.
        const auto engaged = engaged_.load();
        const auto quiet = writes < release_quiet;
        const auto draining = engaged && quiet;
        if (!scarcity && !draining &&
            ((still < idle_seconds) || (transferred == top)))
            continue;

        // A write-hot instance neither transfers nor releases under
        // scarcity: transferred pages re-dirty immediately (write
        // amplification without release payoff, as hash-scattered writes
        // into released pages each cost a segment restore, and the sweep
        // otherwise re-releases restored segments every pass). Its
        // anonymous set is left to swap (dirty-exempt) until quiescence,
        // typically the phase change.
#if !defined(HAVE_APPLE)
        // EXPERIMENT: darwin releases while write-hot (page-level intent and
        // dirty filters remain) to price segment restores against the
        // measured compressed-hot crawl.
        if (scarcity && !quiet)
            continue;
#endif

        {
            std::shared_lock map_lock(remap_mutex_);

            if (!loaded_.load() || fault_.load())
                continue;

            if constexpr (head_release)
                if (scarcity && !engaged)
                    engaged_.store(true);

            // Scarcity passes pace writeback for release: settle maps page
            // cache content, and durability remains a clean close property,
            // so sync applies only to idle draining.
            if (!transfer_<zero>(to_width<zero>(logical_.load())) ||
                (!scarcity && !sync_<zero>()))
            {
                set_first_code(error::fsync_failure);
                continue;
            }

            // Discard the page cache copy of the transfer: the anonymous
            // head is the live copy, so caching the file doubles it.
            file_discard(opened_[zero]);

            // Quiet is assured here (hot scarcity skipped above, and idle
            // draining implies sixty still seconds).
            // Lazy head load engages restore on all platforms; the release
            // sweep remains a darwin response (see head_release).
            if constexpr (head_release)
                if (engaged && !release_pages_())
                    continue;
        }

        transferred = top;
        still = zero;
    }
}

// head page release, synchronized with writers by the bit protocol.
// ----------------------------------------------------------------------------
// private

// Declare the full-page prefix below logical released and engage the restore
// protocol (prepare restores a released segment before its write).
TEMPLATE
void CLASS::declare_released_() NOEXCEPT
{
    using namespace system;
    const auto flags = page_floor(to_width<zero>(logical_.load())) / page_;
    for (size_t word{}; word < ceilinged_divide(flags, page_bound); ++word)
    {
        const auto first = word * page_bound;
        released_[word].store((flags >= (first + page_bound)) ?
            bit_all<uint64_t> : unmask_right<uint64_t>(flags - first));
    }

    lazy_.store(true);
}

// Release cold clean head page runs to read-only file mappings (reclaimable),
// full pages below logical only. Conversion is run-granular (release_chunk
// minimum) as each conversion splits a mapping: page granularity fragments
// the address space beyond what host memory management tolerates. Writer
// synchronization is a per-page bit protocol: prepare() declares intent then
// loads released; release stores released then loads intent (both
// sequentially consistent), so a run converts only when no write can land on
// it unrestored. A wrong release costs one restore. Conversion and restore
// serialize on restore_mutex_.
TEMPLATE
bool CLASS::release_pages_() NOEXCEPT
{
    using namespace system;
    const auto bytes = to_width<zero>(logical_.load());
    const auto pages = bytes / page_;
    const auto bound = std::min(words_, ceilinged_divide(pages, page_bound));
    const auto chunk = std::max(one, release_chunk / page_);

    std::unique_lock restore_lock(restore_mutex_);

    // Materialize candidacy (hot aging clears only the snapshot bits, so a
    // concurrent declaration on a candidate page is retained for the live
    // rechecks below).
    for (size_t word{}; word < bound; ++word)
    {
        const auto hot = intent_[word].load();
        intent_[word].fetch_and(bit_not(hot));
        sweep_[word] = release_below(release_candidates(
            dirty_[word].load(relaxed), hot, released_[word].load(relaxed)),
            pages, word * page_bound);
    }

    // Convert maximal candidate runs of at least chunk pages, clamped to
    // chunk bounds. Restore is chunk-aligned and chunk-granular, so a
    // partially released chunk places unreleased pages inside a restored
    // span. The bit protocol guards released pages only: a writer to an
    // unreleased page observes released clear, so it neither restores nor
    // takes restore_mutex_, and its write is dropped by the copy-install
    // window. Whole chunks make the restored span exactly the released set.
    for (auto run = next_run(sweep_.get(), pages, zero); run.first < pages;
        run = next_run(sweep_.get(), pages, run.second))
    {
        const auto first = ceilinged_divide(run.first, chunk) * chunk;
        const auto second = (run.second / chunk) * chunk;

        if ((second <= first) || ((second - first) < chunk))
            continue;

        const auto begin = first / page_bound;
        const auto end = sub1(second) / page_bound;
        const auto mask = [&](size_t word) NOEXCEPT
        {
            return page_mask(first, second, word * page_bound);
        };

        // Declare the release (prepare() restores from this point).
        for (auto word = begin; word <= end; ++word)
            released_[word].fetch_or(mask(word));

        // An in-flight writer (counted, unaged) or raced intent or mark
        // invalidates the conversion (whole run). The count loads first: a
        // writer counted later observes released and restores, one drained
        // earlier has published its marks (both sequentially consistent).
        auto raced = is_nonzero(writers_count_());
        for (auto word = begin; (word <= end) && !raced; ++word)
            raced = !is_zero(bit_and(mask(word),
                bit_or(intent_[word].load(), dirty_[word].load())));

        if (!raced)
            mmap_unwire(std::next(memory_map_[zero], first * page_),
                (second - first) * page_);

        if (raced || (mmap_settle(
            std::next(memory_map_[zero], first * page_),
            (second - first) * page_, opened_[zero],
            first * page_) == fail))
        {
            for (auto word = begin; word <= end; ++word)
                released_[word].fetch_and(bit_not(mask(word)));

            if (!raced)
            {
                set_first_code(error::mmap_failure);
                return false;
            }
        }
    }

    return true;
}

// Restore released pages overlapping [offset, offset+size) to writable
// anonymous memory (content preserved by atomic installation), before a
// declared write. Restoration is segmented at release_chunk alignment: the
// containing segment restores whole (unifying any fragmentation within it)
// but never more, as released runs consolidate without bound and restoring
// a maximal run copies gigabytes per scattered write (a restore convoy).
TEMPLATE
void CLASS::restore_(size_t offset, size_t size) NOEXCEPT
{
    using namespace system;
    std::unique_lock restore_lock(restore_mutex_);

    // Segments clamp to full pages below logical (as does release candidacy):
    // the reservation above commitment is inaccessible (installation reads).
    const auto pages = to_width<zero>(logical_.load()) / page_;
    const auto span = std::max(one, release_chunk / page_);
    const auto last = (offset + sub1(size)) / page_;
    auto page = offset / page_;
    page -= (page % span);

    for (; (page <= last) && (page < pages); page += span)
    {
        const auto stop = std::min(page + span, pages);
        const auto mask = [&](size_t word) NOEXCEPT
        {
            return page_mask(page, stop, word * page_bound);
        };

        const auto begin = page / page_bound;
        const auto end = sub1(stop) / page_bound;
        auto any = false;
        for (auto word = begin; (word <= end) && !any; ++word)
            any = !is_zero(bit_and(released_[word].load(), mask(word)));

        if (!any)
            continue;

        if (mmap_restore(std::next(memory_map_[zero], page * page_),
            (stop - page) * page_) == fail)
        {
            set_first_code(error::mmap_failure);
            return;
        }

        for (auto word = begin; word <= end; ++word)
            released_[word].fetch_and(bit_not(mask(word)));
    }
}

// head share transitions, synchronized with writers by the count.
// ----------------------------------------------------------------------------
// private

TEMPLATE
std::atomic<size_t>& CLASS::writer_slot_() NOEXCEPT
{
    static std::atomic<size_t> threads{};
    static const thread_local size_t slot = threads.fetch_add(one) %
        writer_shards;
    return writers_.at(slot).count;
}

TEMPLATE
size_t CLASS::writers_count_() const NOEXCEPT
{
    size_t count{};
    for (const auto& shard: writers_)
        count += shard.count.load();

    return count;
}

TEMPLATE
void CLASS::writers_reset_() NOEXCEPT
{
    for (auto& shard: writers_)
        shard.count.store(zero);
}

// Exclude head writers across a share transition: they write through raw
// pointers under no lock, so only the writer count can exclude them.
TEMPLATE
void CLASS::quiesce_() NOEXCEPT
{
    transition_.store(true);
    while (!is_zero(writers_count_()))
        std::this_thread::yield();
}

// Settle a quiescent managed head to a writable mapping of its file over the
// committed span (as a shared head loads): pages drop under pressure and
// writes dirty page cache for kernel writeback, so page tracking idles (it
// remains allocated, as writers read it unlocked). Exclusive remap excludes
// accessors and the transition excludes counted head writers (raw pointer
// writes hold no lock), so a mark count still at the drained count proves
// the file current (a write landing after the drain snapshot would otherwise
// be lost to the remap).
// The remap lock precedes the drain: a writer holding an accessor waits
// uncounted at the transition, so a drain that preceded the lock would return
// on its own and the lock would then wait on that writer forever. Every
// counted writer either holds no lock or took its accessor before its count,
// so under the lock the count drains.
TEMPLATE
bool CLASS::share_(size_t transferred) NOEXCEPT
{
    std::unique_lock map_lock(remap_mutex_);
    quiesce_();

    auto shared = false;
    if (loaded_.load() && !fault_.load() && (marks_.load() == transferred))
    {
        // The drain transfers below logical; the committed fill above it is
        // content (a raised logical exposes it unwritten).
        const auto logical = to_width<zero>(logical_.load());
        const auto span = to_width<zero>(capacity_.load());
        const auto persisted = (span <= logical) || pwrite_all(opened_[zero],
            std::next(memory_map_[zero], logical), span - logical, logical);

        shared = persisted && (mmap_share(memory_map_[zero], span,
            opened_[zero], zero) != fail);

        if (!persisted)
            set_first_code(error::fsync_failure);
        else if (!shared)
            set_first_code(error::mmap_failure);
#if !defined(WITHOUT_MADVISE)
        else if (!advise_(memory_map_[zero], span))
            set_first_code(error::madvise_failure);
#endif
    }

    shared_.store(shared);
    transition_.store(false);
    return shared;
}

// Return a settled head to the anonymous model without a bulk remap: every
// full page declares released (the shared mapping is droppable file content),
// so tracked writers restore segments to anonymous before writing, and the
// tail above the full-page floor restores here (as the load commits it), so
// no dirty page is ever transferred from the file mapping (a self-copy, which
// darwin serves as an uninterruptible wait). Cold pages remain mapped.
TEMPLATE
void CLASS::unshare_() NOEXCEPT
{
    std::unique_lock map_lock(remap_mutex_);
    quiesce_();

    const auto floor = page_floor(to_width<zero>(logical_.load()));
    const auto ceiling = page_ceiling(to_width<zero>(capacity_.load()));
    const auto restored = (ceiling <= floor) || (mmap_restore(
        std::next(memory_map_[zero], floor), ceiling - floor) != fail);

    if (restored)
    {
        declare_released_();
        shared_.store(false);
    }
    else
    {
        set_first_code(error::mmap_failure);
    }

    transition_.store(false);
}

#endif // MANAGE_STAGING

} // namespace database
} // namespace libbitcoin

#endif
