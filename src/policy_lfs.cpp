/*
  Copyright (c) 2016, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <vector>

#include "fs_path.hpp"
#include "policy.hpp"
#include "success_fail.hpp"

using std::string;
using std::vector;
using std::size_t;
using mergerfs::Policy;
using mergerfs::Category;

static
void
_calc_lfs(const struct statvfs  &fsstats,
          const string          *basepath,
          const size_t           minfreespace,
          fsblkcnt_t            &lfs,
          const string         *&lfsbasepath)
{
  fsblkcnt_t spaceavail;

  spaceavail = (fsstats.f_frsize * fsstats.f_bavail);
  if((spaceavail > minfreespace) && (spaceavail < lfs))
    {
      lfs         = spaceavail;
      lfsbasepath = basepath;
    }
}

static
int
_lfs(const vector<string>  &basepaths,
     const char            *fusepath,
     const size_t           minfreespace,
     vector<const string*> &paths)
{
  int rv;
  string fullpath;
  struct statvfs fsstats;
  fsblkcnt_t lfs;
  const string *lfsbasepath;

  lfs = -1;
  lfsbasepath = NULL;
  for(size_t i = 0, ei = basepaths.size(); i != ei; i++)
    {
      const string *basepath = &basepaths[i];

      fs::path::make(basepath,fusepath,fullpath);

      rv = ::statvfs(fullpath.c_str(),&fsstats);
      if(STATVFS_SUCCEEDED(rv))
        _calc_lfs(fsstats,basepath,minfreespace,lfs,lfsbasepath);
    }

  if(lfsbasepath == NULL)
    return (errno=ENOENT,POLICY_FAIL);

  paths.push_back(lfsbasepath);

  return POLICY_SUCCESS;
}

namespace mergerfs
{
  int
  Policy::Func::lfs(const Category::Enum::Type  type,
                    const vector<string>       &basepaths,
                    const char                 *fusepath,
                    const size_t                minfreespace,
                    vector<const string*>      &paths)
  {
    int rv;
    const char *fp =
      ((type == Category::Enum::create) ? "" : fusepath);

    rv = _lfs(basepaths,fp,minfreespace,paths);
    if(POLICY_FAILED(rv))
      rv = Policy::Func::mfs(type,basepaths,fusepath,minfreespace,paths);

    return rv;
  }
}
