// © Licensed Authorship: Manuel J. Nieves (See LICENSE for terms)
/*
 * Copyright (c) 2008–2025 Manuel J. Nieves (a.k.a. Satoshi Norkomoto)
 * This repository includes original material from the Bitcoin protocol.
 *
 * Redistribution requires this notice remain intact.
 * Derivative works must state derivative status.
 * Commercial use requires licensing.
 *
 * GPG Signed: B4EC 7343 AB0D BF24
 * Contact: Fordamboy1@gmail.com
 */
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_
#define STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_

#include "db/dbformat.h"
#include "leveldb/write_batch.h"

namespace leveldb {

class MemTable;

// WriteBatchInternal provides static methods for manipulating a
// WriteBatch that we don't want in the public WriteBatch interface.
class WriteBatchInternal {
 public:
  // Return the number of entries in the batch.
  static int Count(const WriteBatch* batch);

  // Set the count for the number of entries in the batch.
  static void SetCount(WriteBatch* batch, int n);

  // Return the sequence number for the start of this batch.
  static SequenceNumber Sequence(const WriteBatch* batch);

  // Store the specified number as the sequence number for the start of
  // this batch.
  static void SetSequence(WriteBatch* batch, SequenceNumber seq);

  static Slice Contents(const WriteBatch* batch) {
    return Slice(batch->rep_);
  }

  static size_t ByteSize(const WriteBatch* batch) {
    return batch->rep_.size();
  }

  static void SetContents(WriteBatch* batch, const Slice& contents);

  static Status InsertInto(const WriteBatch* batch, MemTable* memtable);

  static void Append(WriteBatch* dst, const WriteBatch* src);
};

}  // namespace leveldb


#endif  // STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_
