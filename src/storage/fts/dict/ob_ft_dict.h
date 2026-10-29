/*
 * Copyright (c) 2025 OceanBase.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _OCEANBASE_STORAGE_FTS_DICT_OB_FT_DICT_H_
#define _OCEANBASE_STORAGE_FTS_DICT_OB_FT_DICT_H_

#include "lib/ob_errno.h"
#include "lib/string/ob_string.h"
#include "storage/fts/dict/ob_ft_dict_def.h"

namespace oceanbase
{
namespace storage
{
class ObIFTDict;
struct ObFTDAT;

// Stack-local cache for one character. Never store mutable lookup state in a shared dict.
class FTDictMatchCache final
{
public:
  explicit FTDictMatchCache(const ObString &single_word)
      : single_word_(single_word), code_count_(0), next_code_(0),
        range_owner_(nullptr), range_dict_(nullptr), range_ret_(OB_SUCCESS)
  {}

  const ObString &single_word() const { return single_word_; }
  int get_code(ObFTDAT *dat, ObFTWordCode &code);

private:
  struct CodeEntry
  {
    ObFTDAT *dat_;
    ObFTWordCode code_;
    int ret_;
  };
  static constexpr int64_t CODE_CACHE_SIZE = 4;
  const ObString single_word_;
  CodeEntry codes_[CODE_CACHE_SIZE];
  int64_t code_count_;
  int64_t next_code_;
  const ObIFTDict *range_owner_;
  ObIFTDict *range_dict_;
  int range_ret_;

  friend class ObFTRangeDict;
};

/**
 * @class ObDATrieHit
 * @brief Record the matching path in a Double-Array-Trie and some info.
 *
 * @desc:
 * - It goes word by word;
 * - Word means a single character in a language;
 */
class ObDATrieHit final
{
public:
  ObDATrieHit(const ObIFTDict *dict, const int64_t start_pos)
      : dict_(dict), char_cnt_(0), start_pos_(start_pos), end_pos_(start_pos_), base_idx_(0),
        current_check_(0), is_match_(false), is_prefix_(false)
  {
  }

  bool is_match() const { return is_match_; }

  bool is_prefix() const { return is_prefix_; }

  bool is_unmatch() const
  {
    bool unmatch = (!is_match_) && (!is_prefix_);
    return unmatch;
  }

  void set_unmatch()
  {
    is_match_ = false;
    is_prefix_ = false;
  }

  void set_match() { is_match_ = true; }

  void set_prefix() { is_prefix_ = true; }

  ObDATrieHit &operator=(const ObDATrieHit &other)
  {
    char_cnt_ = other.char_cnt_;
    start_pos_ = other.start_pos_;
    end_pos_ = other.end_pos_;
    base_idx_ = other.base_idx_;
    current_check_ = other.current_check_;
    is_match_ = other.is_match_;
    is_prefix_ = other.is_prefix_;
    return *this;
  }

  bool operator==(const ObDATrieHit &other) const
  {
    return start_pos_ == other.start_pos_ && end_pos_ == other.end_pos_
           && base_idx_ == other.base_idx_ && current_check_ == other.current_check_;
  }

public:
  const ObIFTDict *dict_;
  // Character count already matched.
  int64_t char_cnt_;

  // The start position in original text.
  int64_t start_pos_;

  // The end position in original text already matched.
  int64_t end_pos_;

  // The base index in Double-Array-Trie.
  uint32_t base_idx_;

  // The current check index in Double-Array-Trie.
  uint32_t current_check_;

  // Status of matching.
  bool is_match_;  // full match
  bool is_prefix_; // prefix match
};

/**
 * @class ObIFTDict
 * @brief Dict interface
 */
class ObIFTDict
{
public:
  ObIFTDict() = default;
  virtual ~ObIFTDict() = default;

  virtual int init() = 0;

  // Match words in dict.
  virtual int match(const ObString &words, bool &is_match) const = 0;

  // Match single word.
  virtual int match(const ObString &single_word, ObDATrieHit &hit) const = 0;

  // Match single word with hit.
  virtual int match_with_hit(const ObString &single_word,
                             const ObDATrieHit &last_hit,
                             ObDATrieHit &hit) const = 0;

  // Reuse character lookups without changing the dispatch of either matching operation.
  virtual int match(FTDictMatchCache &cache, ObDATrieHit &hit) const
  {
    return match(cache.single_word(), hit);
  }
  virtual int match_with_hit(FTDictMatchCache &cache,
                             const ObDATrieHit &last_hit,
                             ObDATrieHit &hit) const
  {
    return match_with_hit(cache.single_word(), last_hit, hit);
  }
};

} // namespace storage
} // namespace oceanbase

#endif // _OCEANBASE_STORAGE_FTS_DICT_OB_FT_DICT_H_
