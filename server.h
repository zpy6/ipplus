#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <stdarg.h>
#include <sys/param.h>

#define MAXBUF          1024
#define ADDR            "10.11.20.95"
#define PORT            1066
#define USER            "nobody"
#define GROUP           "nobody"
#define MAXEPOLLSIZE    65530
#define APPTITLE        "Iplocator"
#define WORKDIR         "/opt/itc/iplocator"

#define MAPFILE         WORKDIR"/map/QQWry.dat"
#define LOGFILE         WORKDIR"/logs/iplocator_log"
#define PIDFILE         WORKDIR"/var/iplocator.pid"

static char* map;
char* init_ip_map();
static int log_fd;
char *strtrim(char *str);
char *stralloc(char *);
void init_daemon(void);
void save_pid(void);
void init_uid(void);
void write_logs(const char *str,...);
