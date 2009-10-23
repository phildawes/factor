#include <unistd.h>
#include <sys/param.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <signal.h>
#include <pthread.h>

namespace factor
{

typedef char vm_char;
typedef char symbol_char;

#define STRING_LITERAL(string) string

#define SSCANF sscanf
#define STRCMP strcmp
#define STRNCMP strncmp
#define STRDUP strdup

#define FTELL ftello
#define FSEEK fseeko

#define FIXNUM_FORMAT "%ld"
#define CELL_FORMAT "%lu"
#define CELL_HEX_FORMAT "%lx"

#ifdef FACTOR_64
	#define CELL_HEX_PAD_FORMAT "%016lx"
#else
	#define CELL_HEX_PAD_FORMAT "%08lx"
#endif

#define FIXNUM_FORMAT "%ld"

#define OPEN_READ(path) fopen(path,"rb")
#define OPEN_WRITE(path) fopen(path,"wb")

#define print_native_string(string) print_string(string)

typedef pthread_t THREADHANDLE;

THREADHANDLE start_thread(void *(*start_routine)(void *),void *args);
inline static THREADHANDLE thread_id() { return pthread_self(); }

void unix_init_signals();
void signal_handler(int signal, siginfo_t* siginfo, void* uap);
void dump_stack_signal(int signal, siginfo_t* siginfo, void* uap);

s64 current_micros();
void sleep_micros(cell usec);

void init_platform_globals();

void register_vm_with_thread(factor_vm *vm);
factor_vm *tls_vm();
void open_console();

struct pthread_size_info {
	int sizeof_pthread_mutex_t;
	int sizeof_pthread_mutexattr_t;
	int sizeof_pthread_cond_t;
	int sizeof_pthread_condattr_t;

	pthread_size_info() 
		: sizeof_pthread_mutex_t(sizeof(pthread_mutex_t)),
		  sizeof_pthread_mutexattr_t(sizeof(pthread_mutexattr_t)),
		  sizeof_pthread_cond_t(sizeof(pthread_cond_t)),
		  sizeof_pthread_condattr_t(sizeof(pthread_condattr_t))
	{}
};

extern "C" const pthread_size_info *pthread_sizes();

}
