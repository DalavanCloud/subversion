/* iter.c : iteration drivers
 *
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 */


#include "svn_iter.h"
#include "svn_pools.h"

#include "svn_error_codes.h"

static svn_error_t internal_break_error =
  {
    SVN_ERR_ITER_BREAK, /* APR status */
    NULL, /* message */
    NULL, /* child error */
    NULL, /* pool */
    __FILE__, /* file name */
    __LINE__ /* line number */
  };

svn_error_t *
svn_iter_apr_hash(svn_boolean_t *completed,
                  apr_hash_t *hash,
                  svn_iter_apr_hash_cb_t func,
                  void *baton,
                  apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  apr_pool_t *iterpool = svn_pool_create(pool);
  apr_hash_index_t *hi;

  for (hi = apr_hash_first(pool, hash);
       ! err && hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      apr_ssize_t len;

      svn_pool_clear(iterpool);

      apr_hash_this(hi, &key, &len, &val);
      err = (*func)(baton, key, len, val, iterpool);
    }

  if (completed)
    *completed = ! err;

  if (err && err->apr_err == SVN_ERR_ITER_BREAK)
    {
      if (err != &internal_break_error)
        /* Errors - except those created by svn_iter_break() -
           need to be cleared when not further propagated. */
        svn_error_clear(err);

      err = SVN_NO_ERROR;
    }

  /* Clear iterpool, because callers may clear the error but have no way
     to clear the iterpool with potentially lots of allocated memory */
  svn_pool_destroy(iterpool);

  return err;
}

svn_error_t *
svn_iter_apr_array(svn_boolean_t *completed,
                   const apr_array_header_t *array,
                   svn_iter_apr_array_cb_t func,
                   void *baton,
                   apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  apr_pool_t *iterpool = svn_pool_create(pool);
  int i;

  for (i = 0; (! err) && i < array->nelts; ++i)
    {
      void *item = array->elts + array->elt_size*i;

      svn_pool_clear(iterpool);

      err = (*func)(baton, item, pool);
    }

  if (completed)
    *completed = ! err;

  if (err && err->apr_err == SVN_ERR_ITER_BREAK)
    {
      if (err != &internal_break_error)
        /* Errors - except those created by svn_iter_break() -
           need to be cleared when not further propagated. */
        svn_error_clear(err);

      err = SVN_NO_ERROR;
    }

  /* Clear iterpool, because callers may clear the error but have no way
     to clear the iterpool with potentially lots of allocated memory */
  svn_pool_destroy(iterpool);

  return err;
}

svn_error_t *
svn_iter__break(void)
{
  return &internal_break_error;
}

/* APR isn't fully constified, and apr_hash_this does not expect a const
 * hash index parameter. However, it does not modify the hash index,
 * and in Subversion we're trying to be const-correct.
 * So these functions all take const hash indices, and we cast the const
 * away when passing them down to APR to avoid compiler warnings. */

const void *svn__apr_hash_index_key(const apr_hash_index_t *hi)
{
  const void *key;

  apr_hash_this((apr_hash_index_t *)hi, &key, NULL, NULL);
  return key;
}

apr_ssize_t svn__apr_hash_index_klen(const apr_hash_index_t *hi)
{
  apr_ssize_t klen;

  apr_hash_this((apr_hash_index_t *)hi, NULL, &klen, NULL);
  return klen;
}

void *svn__apr_hash_index_val(const apr_hash_index_t *hi)
{
  void *val;

  apr_hash_this((apr_hash_index_t *)hi, NULL, NULL, &val);
  return val;
}
