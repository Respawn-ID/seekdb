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

#include "data_plane/fts/ob_fts_struct.h"

#include "lib/charset/ob_charset.h"
#include "share/datum/ob_datum_funcs.h"
#include "storage/ob_storage_util.h"

namespace oceanbase
{
namespace storage
{

namespace
{
common::ObDatumBasicFuncs *get_word_basic_funcs(const ObObjMeta &meta)
{
  struct CacheEntry
  {
    common::ObObjType type_;
    common::ObCollationType collation_;
    common::ObDatumBasicFuncs *funcs_;
  };
  // The four supported collations occupy distinct slots. Cache both the word
  // and stopword function tables without enlarging map keys.
  // The datum function tables have process lifetime; only the cache is mutable.
  static thread_local CacheEntry cache[4] = {};
  const common::ObObjType type = meta.get_type();
  const common::ObCollationType collation = meta.get_collation_type();
  CacheEntry &entry = cache[static_cast<uint64_t>(collation) % ARRAYSIZEOF(cache)];
  if (OB_ISNULL(entry.funcs_) || entry.type_ != type || entry.collation_ != collation) {
    entry.funcs_ = common::ObDatumFuncs::get_basic_func(type, collation);
    entry.type_ = type;
    entry.collation_ = collation;
  }
  return entry.funcs_;
}
} // namespace

int ObFTWord::hash(uint64_t &hash_val) const
{
  int ret = OB_SUCCESS;
  common::ObDatumBasicFuncs *funcs = get_word_basic_funcs(meta_);
  if (OB_ISNULL(funcs)) {
    ret = OB_ERR_UNEXPECTED;
  } else if (funcs->default_hash_ == nullptr) {
    ret = OB_ERR_UNEXPECTED;
  } else {
    ret = funcs->default_hash_(word_, 0, hash_val, nullptr);
  }
  return ret;
}
bool ObFTWord::operator==(const ObFTWord &other) const
{
  bool is_equal = false;
  int ret = OB_SUCCESS;
  int cmp_ret = 0;
  // Equal types take the basic-function branch in get_datum_cmp_func().
  ObDatumCmpFuncType func = meta_.get_type() == other.meta_.get_type()
      ? get_word_basic_funcs(meta_)->null_first_cmp_
      : get_datum_cmp_func(meta_, other.meta_);
  if (func == nullptr) {
    ob_abort();
  } else if (OB_FAIL(func(word_, other.word_, cmp_ret, nullptr))) {
    ob_abort();
  } else {
    is_equal = (cmp_ret == 0);
  }
  return is_equal;
}
} // namespace storage
} // namespace oceanbase
