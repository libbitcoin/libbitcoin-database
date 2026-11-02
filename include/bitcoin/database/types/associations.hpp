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
#ifndef LIBBITCOIN_DATABASE_TYPES_ASSOCIATIONS_HPP
#define LIBBITCOIN_DATABASE_TYPES_ASSOCIATIONS_HPP

#include <set>
#include <unordered_map>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/association.hpp>

namespace libbitcoin {
namespace database {

/// Collection of association between block hash and context.
/// Indexed by unique block hash and by unique block height.
class associations
{
public:
    /// Height order, with heterogeneous lookup by height.
    struct by_height
    {
        using is_transparent = void;

        inline bool operator()(const association& left,
            const association& right) const NOEXCEPT
        {
            return left.context.height < right.context.height;
        }

        inline bool operator()(const association& left,
            size_t right) const NOEXCEPT
        {
            return left.context.height < right;
        }

        inline bool operator()(size_t left,
            const association& right) const NOEXCEPT
        {
            return left < right.context.height;
        }
    };

    using container = std::set<association, by_height>;
    using iterator = container::const_iterator;
    using const_iterator = container::const_iterator;

    DEFAULT_COPY_MOVE(associations);

    associations() NOEXCEPT = default;

    template <typename Iterator>
    inline associations(Iterator first, Iterator last) NOEXCEPT
    {
        std::for_each(first, last, [this](const association& item) NOEXCEPT
        {
            insert(item);
        });
    }

    /// Insert unless either the hash or the height is present.
    inline std::pair<const_iterator, bool> insert(
        const association& item) NOEXCEPT
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        if (exists(item.hash))
            return { find(item.hash), false };

        const auto result = items_.insert(item);
        if (result.second)
            hashes_.emplace(item.hash, item.context.height);

        return result;
        BC_POP_WARNING()
    }

    /// Remove the element from both indexes.
    inline const_iterator erase(const_iterator it) NOEXCEPT
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        hashes_.erase(it->hash);
        return items_.erase(it);
        BC_POP_WARNING()
    }

    /// Move the elements of other not present here (others remain in other).
    inline void merge(associations& other) NOEXCEPT
    {
        for (auto it = other.begin(); it != other.end();)
        {
            if (insert(*it).second)
                it = other.erase(it);
            else
                ++it;
        }
    }

    /// Move the count lowest heights into the returned collection.
    inline associations extract(size_t count) NOEXCEPT
    {
        associations out{};
        while (is_nonzero(count--) && !empty())
        {
            const auto it = begin();
            out.insert(*it);
            erase(it);
        }

        return out;
    }

    inline void clear() NOEXCEPT
    {
        items_.clear();
        hashes_.clear();
    }

    inline size_t size() const NOEXCEPT
    {
        return items_.size();
    }

    inline bool empty() const NOEXCEPT
    {
        return items_.empty();
    }

    /// Iteration is in height order.
    inline const_iterator begin() const NOEXCEPT
    {
        return items_.cbegin();
    }

    inline const_iterator end() const NOEXCEPT
    {
        return items_.cend();
    }

    inline const_iterator cbegin() const NOEXCEPT
    {
        return items_.cbegin();
    }

    inline const_iterator cend() const NOEXCEPT
    {
        return items_.cend();
    }

    /// Forward iterator for the height index.
    inline const_iterator pos_begin() const NOEXCEPT
    {
        return items_.cbegin();
    }

    /// Forward end iterator for the height index.
    inline const_iterator pos_end() const NOEXCEPT
    {
        return items_.cend();
    }

    /// Returns an iterator to an association instance, or .end().
    inline const_iterator find(const system::hash_digest& hash) const NOEXCEPT
    {
        const auto it = hashes_.find(hash);
        return it == hashes_.end() ? end() : items_.find(it->second);
    }

    /// Returns an iterator to an association instance, or .pos_end().
    inline const_iterator find(size_t height) const NOEXCEPT
    {
        return items_.find(height);
    }

    /// Hash identifies an element in the hash index.
    inline bool exists(const system::hash_digest& hash) const NOEXCEPT
    {
        return hashes_.contains(hash);
    }

    /// Height identifies an element in the height index.
    inline bool exists(size_t height) const NOEXCEPT
    {
        return items_.contains(height);
    }

    /// The context of the maximum height (must not be empty).
    inline const system::chain::context& top() const NOEXCEPT
    {
        BC_ASSERT(!empty());
        return items_.crbegin()->context;
    }

private:
    container items_{};
    std::unordered_map<system::hash_digest, size_t> hashes_{};
};

} // namespace database
} // namespace libbitcoin

#endif
