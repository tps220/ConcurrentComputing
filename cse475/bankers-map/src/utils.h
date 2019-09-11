#include <mutex>

#define LOCK_TYPE std::mutex g_num_mutex;
#define LOCK(mutex) ((mutex).lock())
#define UNLOCK(mutex) ((mutex).unlock())
#define INTIALIZE_LOCK(mutex)