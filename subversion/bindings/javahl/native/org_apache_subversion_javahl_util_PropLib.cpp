/**
 * @copyright
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
 * @endcopyright
 *
 * @file org_apache_subversion_javahl_util_PropLib.cpp
 * @brief Implementation of the native methods in the Java class PropLib
 */

#include <iostream>
#include <sstream>

#include "../include/org_apache_subversion_javahl_util_PropLib.h"

#include "JNIStackElement.h"
#include "JNIStringHolder.h"
#include "JNIByteArray.h"
#include "JNIUtil.h"
#include "InputStream.h"
#include "EnumMapper.h"
#include "Path.h"
#include "Pool.h"

#include "svn_props.h"
#include "svn_time.h"
#include "svn_wc.h"

#include "svn_private_config.h"


namespace {
class PropGetter
{
public:
  PropGetter(const char* mime_type, svn_stream_t* contents)
    : m_mime_type(mime_type),
      m_contents(contents)
    {}

  static svn_error_t* callback(const svn_string_t** mime_type,
                               svn_stream_t* stream, void* baton,
                               apr_pool_t* pool)
    {
      PropGetter* self = static_cast<PropGetter*>(baton);
      if (mime_type)
        {
          if (self->m_mime_type)
            *mime_type = svn_string_create(self->m_mime_type, pool);
          else
            *mime_type = svn_string_create_empty(pool);
        }

      if (stream && self->m_contents)
        {
          SVN_ERR(svn_stream_copy3(self->m_contents,
                                   svn_stream_disown(stream, pool),
                                   NULL, NULL, pool));
        }

      return SVN_NO_ERROR;
    }

private:
  const char* m_mime_type;
  svn_stream_t* m_contents;
};
} // anonymous namespace


JNIEXPORT jbyteArray JNICALL
Java_org_apache_subversion_javahl_util_PropLib_checkNodeProp(
    JNIEnv* env, jobject jthis,
    jstring jname, jbyteArray jvalue, jstring jpath, jobject jkind,
    jstring jmime_type, jobject jfile_contents,
    jboolean jskip_some_checks)
{
  JNIEntry(PropLib, checkLocalProp);

  JNIStringHolder name(jname);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  JNIByteArray value(jvalue);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  JNIStringHolder path(jpath);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  svn_node_kind_t kind = EnumMapper::toNodeKind(jkind);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  JNIStringHolder mime_type(jmime_type);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  InputStream contents(jfile_contents);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  // Using a "global" request pool since we don't keep a context with
  // its own pool around for these functions.
  SVN::Pool pool;

  PropGetter getter(mime_type.c_str(),
                    (jfile_contents ? contents.getStream(pool) : NULL));

  svn_string_t propval;
  propval.data = reinterpret_cast<const char*>(value.getBytes());
  propval.len = value.getLength();

  const svn_string_t* canonval;
  SVN_JNI_ERR(svn_wc_canonicalize_svn_prop(
                  &canonval, name.c_str(), &propval, path.c_str(),
                  kind, svn_boolean_t(jskip_some_checks),
                  PropGetter::callback, &getter,
                  pool.getPool()),
              NULL);

  return JNIUtil::makeJByteArray(canonval->data, int(canonval->len));
}


#include "jniwrapper/jni_stack.hpp"
#include "jniwrapper/jni_array.hpp"
#include "jniwrapper/jni_list.hpp"
#include "ExternalItem.hpp"
#include "SubversionException.hpp"


namespace {
struct FormatRevision
{
  explicit FormatRevision(const svn_opt_revision_t* const& revarg,
                          const SVN::Pool& poolarg)
    : rev(revarg), pool(poolarg)
    {}

  const svn_opt_revision_t* const& rev;
  const SVN::Pool& pool;
};

std::ostream& operator<<(std::ostream& os, const FormatRevision& pr)
{
  switch (pr.rev->kind)
    {
    case svn_opt_revision_number:
      os << pr.rev->value.number;
      break;
    case svn_opt_revision_date:
      os << '{'
         << svn_time_to_cstring(pr.rev->value.date, pr.pool.getPool())
         << '}';
      break;
    default:
      throw std::logic_error(
          _("Invalid revision tag; must be a number or a date"));
    }
  return os;
}
} // anoymous namespace


JNIEXPORT jobject JNICALL
Java_org_apache_subversion_javahl_util_PropLib_parseExternals(
    JNIEnv* jenv, jobject jthis,
    jbyteArray jdescription, jstring jparent_dir, jboolean jcanonicalize_url)
{
  SVN_JAVAHL_JNI_TRY(PropLib, parseExternals)
    {
      const Java::Env env(jenv);

      const Java::ByteArray description(env, jdescription);
      const Java::String paren_dir(env, jparent_dir);
    }
  SVN_JAVAHL_JNI_CATCH;
  return NULL;
}

JNIEXPORT jbyteArray JNICALL
Java_org_apache_subversion_javahl_util_PropLib_unparseExternals(
    JNIEnv* jenv, jobject jthis,
    jobject jitems, jstring jparent_dir, jboolean jold_format)
{
  SVN_JAVAHL_JNI_TRY(PropLib, unparseExternals)
    {
      const Java::Env env(jenv);

      const Java::List<JavaHL::ExternalItem> items(env, jitems);
      const Java::String parent_dir(env, jparent_dir);

      // Using a "global" iteration pool since we don't keep a context
      // with its own pool around for these functions.
      SVN::Pool iterpool;

      std::ostringstream buffer;
      for (jint i = 0; i < items.length(); ++i)
        {
          iterpool.clear();

          const Java::LocalFrame frame(env);
          const JavaHL::ExternalItem item(items[i]);

          if (!jold_format)
            {
              if (item.revision()->kind != svn_opt_revision_head)
                {
                  buffer << "-r"
                         << FormatRevision(item.revision(), iterpool)
                         << ' ';
                }
              if (item.peg_revision()->kind == svn_opt_revision_head)
                buffer << item.url() << ' ';
              else
                {
                  buffer << item.url() << '@'
                         << FormatRevision(item.peg_revision(), iterpool)
                         << ' ';
                }
              buffer << item.target_dir() << '\n';
            }
          else
            {
              // Sanity check: old format does not support peg revisions
              if (item.peg_revision()->kind != svn_opt_revision_head)
                {
                  JavaHL::SubversionException(env)
                    .raise(_("Clients older than Subversion 1.5"
                             " do not support peg revision syntax"
                             " in the svn:externals property"));
                }

              // Sanity check: old format does not support relative URLs
              const std::string url = item.url();
              if (   (url.size() >= 1 && (url[0] == '.' || url[0] == '/'))
                  || (url.size() >= 2 && (url[0] == '^' && url[1] == '/')))
                {
                  JavaHL::SubversionException(env)
                    .raise(_("Clients older than Subversion 1.5"
                             " do not support relative URLs"
                             " in the svn:externals property"));
                }

              buffer << item.target_dir() << ' ';
              if (item.revision()->kind != svn_opt_revision_head)
                {
                  buffer << "-r"
                         << FormatRevision(item.revision(), iterpool)
                         << ' ';
                }
              buffer << url << '\n';
            }
        }

      // Validate the result. Even though we generated the string
      // ourselves, we did not validate the input paths and URLs.
      const std::string description(buffer.str());
      SVN_JAVAHL_CHECK(svn_wc_parse_externals_description3(
                           NULL,
                           Java::String::Contents(parent_dir).c_str(),
                           description.c_str(),
                           false, iterpool.getPool()));
      return Java::ByteArray(env, description).get();
    }
  SVN_JAVAHL_JNI_CATCH;
  return NULL;
}
