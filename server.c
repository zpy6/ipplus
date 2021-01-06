#include  "conf.h"
#include "server.h"
#include "iplocator.h"

char *bindaddress;
int bindport;
char *path;
char *user;
char *group;
char *logfile;
char *pidfile;
char *datfile;

char *stralloc(char *str)
{
	int len = strlen(str);
	char *ret = (char *)calloc(len + 1, 1);
	if (ret == NULL) {
		write_logs("LOG_ERR can't alloc memory for string");
	} else {
		strncpy(ret, str, len + 1);
	}
	return ret;
}

char *strtrim(char *str) {
	int len = strlen(str);
	char *p = str + len;
	while (p != str) {      /* trim the white space at the end of string */
		p--;
		if (isspace(*p) || *p=='\n' || *p=='\r')
			*p = '\0';
		else
			break;
	}
	p = str;
	while (*p != '\0') {    /* trim the beginning white space */
		if (isspace(*p))
			p ++;
		else
			break;
	}
	return p;
}

char* init_ip_map() {
	int fd;
	struct stat sb;
	if((fd = open(datfile,O_RDONLY)) < 0) {
		write_logs("Unable to open file: %s\n", MAPFILE);
		exit(-1);
	}
	if (fstat(fd,&sb)) {
		write_logs("fstat ipdata error");
		exit(1);
	}
	char *mm = (char *)mmap(NULL,sb.st_size,PROT_READ,MAP_SHARED,fd,0);
	if (mm == NULL ) {
		write_logs("mmap ipdata error");
		exit(1);
	}
	close(fd);
	return mm;
}

void write_logs(const char *format,...)
{
	ssize_t wrote;
	ssize_t len;
	time_t cur_time;
	struct tm tm;
	char buf[4096],*p;
	va_list arglist;

	time(&cur_time);
	localtime_r(&cur_time,&tm);
	strftime(buf,sizeof(buf),"%F %T",&tm);
	strncat(buf," ",1);
	len=strlen(buf);
	p=buf+len;
	len=sizeof(buf)-len;
	va_start(arglist, format);
	vsnprintf(p, len, format, arglist);
	va_end(arglist);
	strncat(buf,"\n",1);
	if ( (write(log_fd,buf,strlen(buf))) <0  ) {
		write_logs("write_logs %d failed\n",log_fd);
		exit(1);
	}
}

void init_daemon(void){
	int i;
	pid_t pid;

	if ((pid = fork()) != 0)
		exit(0);
	setsid();
	if ((pid = fork()) != 0)
		exit(0);
	for(i=0;i<3;i++)
		close(i);
	chdir("/");
	umask(0);
}

void save_pid(void)
{
	char buf[32];
	int  fd;
	if ( (fd=open(pidfile,O_CREAT|O_WRONLY|O_TRUNC,0644))<0 ) {
		write_logs("can't open %s",PIDFILE);
		exit(1);
	}
	snprintf(buf, sizeof(buf), "%d", getpid());
	write(fd,buf,strlen(buf));
	close(fd);
}

void init_uid(void)
{
	uid_t uid;
	gid_t gid;
	struct passwd *pwd;
	struct group *grp;
	//pwd = getpwnam(USER);
	pwd = getpwnam(user);
	if (pwd == NULL) {
		write_logs("no such user");
		exit(1);
	}
	uid = pwd->pw_uid;
	//grp = getgrnam(GROUP);
	grp = getgrnam(group);
	if (grp == NULL) {
		write_logs("no such group: exit");
		exit(1);
	}
	gid = grp->gr_gid;
	setregid(gid, gid);
	setreuid(uid, uid);
}

int setnonblocking(int sockfd) {
	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1)
		return -1;
	return 0;
}

int handle_message(int new_fd,char *client_ip) {
	char buf[MAXBUF+1];
	char str[MAXBUF+1];
	char r[MAXBUF+1];
	char p[MAXBUF+1];
	int  a,b,c,d;
	int len;
	struct timeval tv1, tv2;
	double clock1, clock2;
	char *err_msg="query string error\n\n";
	char *pp,*line;
	char Protocol[8];

	bzero(buf,MAXBUF+1);
	len=recv(new_fd, buf, MAXBUF, 0);
	line=strtrim(buf);
	if (len>0) {
		if (sscanf(line,"GET %d.%d.%d.%d",&a,&b,&c,&d)==4) {
			snprintf(p,sizeof(p),"%d.%d.%d.%d",a,b,c,d);
			gettimeofday(&tv1, NULL);
			search(map,p,r);
			gettimeofday(&tv2, NULL);
			clock1 = (double)tv1.tv_sec + (double)tv1.tv_usec / 1000000;
			clock2 = (double)tv2.tv_sec + (double)tv2.tv_usec / 1000000;
			pp=strtrim(r);
			send(new_fd,pp,strlen(pp),0);
			write_logs("%s query:%s get:\"%s\" stime:%lf",client_ip,p,r,clock2-clock1);
		} else if (sscanf(line,"GET /%d.%d.%d.%d %[^ ]",&a,&b,&c,&d,Protocol)==5) {
			snprintf(p,sizeof(p),"%d.%d.%d.%d",a,b,c,d);
			gettimeofday(&tv1, NULL);
			search(map,p,r);
			gettimeofday(&tv2, NULL);
			clock1 = (double)tv1.tv_sec + (double)tv1.tv_usec / 1000000;
			clock2 = (double)tv2.tv_sec + (double)tv2.tv_usec / 1000000;
			pp=strtrim(r);
			snprintf(str,sizeof(str),"%s %d OK\n",Protocol,200);
			send(new_fd,str,strlen(str),0);
			send(new_fd,"Content-type: text/html\n\n",strlen("Content-type: text/html\n\n"),0);
			send(new_fd,pp,strlen(pp),0);
			write_logs("%s query:%s get:\"%s\" stime:%lf",client_ip,p,r,clock2-clock1);
			close(new_fd);

		} else if (strncmp(line,"quit",4)==0) {
			send(new_fd,"bye",strlen("bye"),0);
			close(new_fd);
		} else if (strncmp(line,"helo",4)==0) {
			send(new_fd,"iplocator v1.0",strlen("iplocator v1.0"),0);
		} else {
			send(new_fd,err_msg,strlen(err_msg),0);
			write_logs("%s query string error",client_ip);
			close(new_fd);
		}
	} else {
		if (len<0) {
			write_logs("recv error\n");
			close(new_fd);
		}
		return -1;
	}
	return len;
}

int main(int argc, char **argv)
{
	char *s;
	if (parse_options(argc, argv))
		exit(1);
	//bindaddress = malloc(sizeof(char*));
	bindaddress = option_get_str("bindaddress");
	path = option_get_str("path");
	bindport = option_get_int("bindport");
	user = option_get_str("user");
	group = option_get_str("group");
	logfile = option_get_str("logfile");
	pidfile = option_get_str("pidfile");
	datfile = option_get_str("datfile");
	printf("iplocator start with params below:\n\tbindip=%s\n\tport=%d\n\tuser=%s\n\tgroup=%s\n\tlog=%s\n\tpid=%s\n\tdat=%s\nsucceed\n\n",bindaddress,bindport,user,group,logfile,logfile,datfile);
	init_daemon();
	init_uid();
	save_pid();
	map=init_ip_map();
	if((log_fd=open(logfile,O_CREAT|O_APPEND|O_WRONLY,0644)) < 0) {
		write_logs("Unable to open file: %s\n", LOGFILE);
		exit(-1);
	}
	int listener, new_fd, kdpfd, nfds, n, ret, curfds;
	socklen_t len;
	struct sockaddr_in my_addr, their_addr;
	struct epoll_event ev;
	struct epoll_event events[MAXEPOLLSIZE];
	struct rlimit rt;

	rt.rlim_max = rt.rlim_cur = MAXEPOLLSIZE;
	if (setrlimit(RLIMIT_NOFILE, &rt) == -1) {
		write_logs("setrlimit to %d failed, exit",RLIMIT_NOFILE);
		exit(1);
	}
	if ((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		write_logs("create socket failed code(%d), exit",errno);
		exit(1);
	}
	setnonblocking(listener);
	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = PF_INET;
	//my_addr.sin_addr.s_addr = inet_addr(ADDR);
	//my_addr.sin_port = htons(PORT);
	my_addr.sin_addr.s_addr = inet_addr(bindaddress);
	my_addr.sin_port = htons(bindport);
	if (bind(listener, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {
		write_logs("bind failed,fail code %d, exit",errno);
		exit(1);
	}
	if (listen(listener,50) == -1) {
		write_logs("listen error");
		exit(1);
	}
	kdpfd = epoll_create(MAXEPOLLSIZE);
	len = sizeof(struct sockaddr_in);
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = listener;
	if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, listener, &ev) < 0) {
		write_logs("epoll set insertion error: fd=%d,code(%d)\n", listener,errno);
		return -1;
	}
	curfds = 1;
	while (1) {
		nfds = epoll_wait(kdpfd, events, curfds, -1);
		if (nfds == -1) {
			write_logs("epoll_wait failed,code(%d), exit",errno);
			break;
		}
		for (n = 0; n <nfds; ++n) {
			if (events[n].data.fd == listener) {
				new_fd = accept(listener, (struct sockaddr *) &their_addr,&len);
				if (new_fd<0) {
					write_logs("accept failed,code(%d), exit",errno);
					continue;
				}
				setnonblocking(new_fd);
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = new_fd;
				if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, new_fd, &ev) < 0) {
					write_logs("add %d into epoll failed",new_fd);
					return -1;
				}
				curfds++;
			} else {
				ret = handle_message(events[n].data.fd,inet_ntoa(their_addr.sin_addr));
				if (ret<1 && errno != 11) {
					epoll_ctl(kdpfd, EPOLL_CTL_DEL, events[n].data.fd,&ev);
					curfds--;
				}
			}
		}
	}
	close(listener);
	close(log_fd);
	return 0;
}
