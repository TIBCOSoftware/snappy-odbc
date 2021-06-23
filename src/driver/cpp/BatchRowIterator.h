/*
 * Copyright (c) 2018 SnappyData, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You
 * may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License. See accompanying
 * LICENSE file.
 */

#ifndef BATCHROWITERATOR_H_
#define BATCHROWITERATOR_H_

#include "ResultSet.h"
#include "UpdatableRow.h"

namespace io {
namespace snappydata {

/**
 * BatchRowIterator.h
 *
 * Given an underlying C++ iterator (possibly cached like ResultSet iterator)
 * provide batch iterator on top having a fixed batch size. It differs from the
 * underlying iterator in providing additional methods to check for batch end
 * and also ensuring that batch is filled as much as possible if there are
 * results available (and a partially filled batch means that whole set of
 * results have been consumed).
 */
class BatchRowIterator final {
public:
  BatchRowIterator(client::ResultSet::iterator& iterator,
      const uint32_t batchSize) : m_iterator(iterator),
      m_position(-1), m_batchSize(batchSize) {
  }

  ~BatchRowIterator() {
  }

  /**
   * Move to the next element in the current batch.
   * Returns true if successful else return false which can mean end
   * of current batch or could mean end of entire result set. One can
   * check for the latter case by invoking {@link nextBatch()}.
   */
  bool next() {
    return ++m_position < static_cast<int32_t>(m_batchSize) && m_iterator.next();
  }

  /**
   * Move to the previous element in the current batch.
   * Returns true if successful else return false which can mean end
   * of current batch or could mean end of entire result set. One can
   * check for the latter case by invoking {@link previousBatch()}.
   */
  bool previous() {
    return --m_position > 0 && m_iterator.previous();
  }

  /**
   * Initialize to fetch the next batch of results (using {@link next()}).
   */
  void initNextBatch() {
    m_position = 0;
  }

  /**
   * Initialize to fetch the previous batch of results (using {@link previous()}).
   */
  void initPreviousBatch() {
    m_position = m_batchSize - 1;
  }

  /** Get the current 0-based position in the batch. */
  inline int32_t position() const noexcept {
    return m_position;
  }

  /** Get the configured batch size. */
  inline uint32_t batchSize() const noexcept {
    return m_batchSize;
  }

  /** Set the batch size. */
  inline void setBatchSize(uint32_t batchSize) noexcept {
    m_batchSize = batchSize;
  }

private:
  client::ResultSet::iterator &m_iterator;
  /**
   * Current 0-based position in the batch. -ve means before first and
   * >= batch size means after last.
   */
  int32_t m_position;
  uint32_t m_batchSize;
};

} /* namespace snappydata */
} /* namespace io */

#endif /* BATCHROWITERATOR_H_ */
