#include "rgw_http_client_ssl.h"
#include <openssl/crypto.h>
#include <pthread.h>

void rgw_ssl_locking_callback(int mode, int id, const char *file, int line)
{
  static RGWSSLSetup locks(CRYPTO_num_locks());
  if (mode & CRYPTO_LOCK)
    locks.set_lock(id);
  else
    locks.clear_lock(id);
}

unsigned long rgw_ssl_thread_id_callback(){
  return (unsigned long)pthread_self();
}
