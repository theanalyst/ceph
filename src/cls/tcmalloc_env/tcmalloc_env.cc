/*
 * rados-classes based plugin to set TCmalloc environment variable
 * 'TCMALLOC_MAX_TOTAL_THREAD_CACHE_BYTES' as the existing tcmalloc
 * library in ubuntu 14.04 LTS and centos 7.0 has a bug which does
 * not honor this env
 */
#include <cstdlib>
#ifdef HAVE_GPERFTOOLS_HEAP_PROFILER_H
#include <gperftools/heap-profiler.h>
#else
#include <google/heap-profiler.h>
#endif

#ifdef HAVE_GPERFTOOLS_MALLOC_EXTENSION_H
#include <gperftools/malloc_extension.h>
#else
#include <google/malloc_extension.h>
#endif
#include "objclass/objclass.h"

CLS_VER (1, 0) CLS_NAME (tcmalloc_env)
#define DEFAULT_CACHE_SIZE (32 * 1024 * 1024)

void
__cls_init ()
{
  size_t result;
  size_t cache_sz;
  char *env_cache_sz_str;

  CLS_LOG (0, "TCMALLOC-ENV: Search");
  env_cache_sz_str = getenv ("TCMALLOC_MAX_TOTAL_THREAD_CACHE_BYTES");
  if (env_cache_sz_str) {
    cache_sz = strtoul (env_cache_sz_str, NULL, 0);
    CLS_LOG(0, "TCMALLOC-ENV: Found: %lu\n", cache_sz);
    if (cache_sz > DEFAULT_CACHE_SIZE) {
      MallocExtension::instance ()->
        SetNumericProperty ("tcmalloc.max_total_thread_cache_bytes", cache_sz);
      result = 0;
      MallocExtension::instance ()->
        GetNumericProperty ("tcmalloc.max_total_thread_cache_bytes", &result);
      CLS_LOG (0, "TCMALLOC-ENV:Verify: max_total_thread_cache_bytes=%lu\n",
        (unsigned long) result);
    } else {
      CLS_LOG (0, "TCMALLOC-ENV: Exported CacheSz=%lu <= Default=%lu - Ignored\n",
        (unsigned long) cache_sz, (unsigned long) DEFAULT_CACHE_SIZE);
    }
  } else {
      CLS_LOG(0, "TCMALLOC-ENV: Not Found\n");
  }
}
