/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TABLES_TABLES_HPP
#define LIBBITCOIN_DATABASE_TABLES_TABLES_HPP

#include <bitcoin/database/tables/archives/header.hpp>
#include <bitcoin/database/tables/archives/input.hpp>
#include <bitcoin/database/tables/archives/output.hpp>
#include <bitcoin/database/tables/archives/point.hpp>
#include <bitcoin/database/tables/archives/puts.hpp>
#include <bitcoin/database/tables/archives/transaction.hpp>
#include <bitcoin/database/tables/archives/txs.hpp>

#include <bitcoin/database/tables/caches/bootstrap.hpp>
#include <bitcoin/database/tables/caches/buffer.hpp>
#include <bitcoin/database/tables/caches/neutrino.hpp>
#include <bitcoin/database/tables/caches/validated_block.hpp>
#include <bitcoin/database/tables/caches/validated_tx.hpp>

#include <bitcoin/database/tables/indexes/address.hpp>
#include <bitcoin/database/tables/indexes/candidate_height.hpp>
#include <bitcoin/database/tables/indexes/confirmed_block.hpp>
#include <bitcoin/database/tables/indexes/confirmed_height.hpp>
#include <bitcoin/database/tables/indexes/confirmed_tx.hpp>
#include <bitcoin/database/tables/indexes/input_tx.hpp>
#include <bitcoin/database/tables/indexes/output_tx.hpp>
#include <bitcoin/database/tables/indexes/spent_output.hpp>

#endif
