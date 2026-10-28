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
#include <bitcoin/database/define.hpp>

// define.hpp is the common include for /database.
// All non-database headers include define.hpp.
// Database inclusions are chained as follows.

// version        : <generated>
// boost          : version <bitcoin/system>
// error          : boost
// define         : error

// Other directory common includes are not internally chained.
// Each header includes only its required common headers.

// /locks         : define
// /file          : define
// /memory        : /file /locks
// /primitives    : /memory
// /tables        : /primitives                    [forward: settings]
// /types         : /tables
// /unspent       : /types                         [forward: query]
// settings       : /types
// query          : /unspent settings
// store          : settings

// When a symbol is unexplainably undefined, its defining include is probably
// creating the circularity.
