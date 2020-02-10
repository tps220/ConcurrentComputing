#include <pthread.h>

#define LOCK_TYPE pthread_mutex_lock
#define LOCK(p) pthread_mutex_lock(&(p))
#define TRY_LOCK(p) pthread_mutex_trylock(&(p))
#define UNLOCK(p) pthread_mutex_unlock(&(p))
#define INIT_LOCK(p) pthread_mutex_init(&(p), NULL)
#define DESTRUCT_LOCK(p) 