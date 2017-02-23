[![Build Status](https://travis-ci.org/libbitcoin/libbitcoin-database.svg?branch=master)](https://travis-ci.org/libbitcoin/libbitcoin-database)

[![Coverage Status](https://coveralls.io/repos/libbitcoin/libbitcoin-database/badge.svg)](https://coveralls.io/r/libbitcoin/libbitcoin-database)

# Libbitcoin Database

*Bitcoin High Performance Blockchain Database*

Make sure you have installed [libbitcoin](https://github.com/libbitcoin/libbitcoin) beforehand according to its build instructions.

```sh
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
$ sudo ldconfig
```

libbitcoin-database is now installed in `/usr/local/`.

**About Libbitcoin Database**

Libbitcoin Database is a custom database build directly on the operating system's [memory-mapped file](https://en.wikipedia.org/wiki/Memory-mapped_file) system. All primary tables and indexes are built on in-memory hash tables, resulting in constant-time lookups. The database uses [sequence locking](https://en.wikipedia.org/wiki/Seqlock) to avoid blocking the writer. This is ideal for a high performance blockchain server as reads are significantly more frequent than writes and yet writes must proceed wtihout delay. The [libbitcoin-blockchain](https://github.com/libbitcoin/libbitcoin-blockchain) library uses the database as its blockchain store.
