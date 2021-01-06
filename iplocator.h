#ifndef _IP_LOCATOR_
#endif

#define _IP_LOCATOR_

/**
 * Define you database file name here.
 */
#define DEFAULT_IPDB_FILE  "QQWry.dat"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 *      Input: 
 *              ip: little endaian
 *              len: the size of 'addr'
 *      Output:
 *              addr: ip address
 *              range_start_ip & range_end_ip: the ip range
 *                      set to NULL to omit it.
 *      return 0 for success. -1 if error.
 */
void search(char*map,char *ip,char *r);
int get_ip_addr(unsigned int ip,
                unsigned char *addr,
                int len,
                unsigned int *range_start_ip,
                unsigned int *range_end_ip
                );
unsigned int str2ip(const char *cs);
char *ip2str(unsigned int ip, char *buf);
void get_version(unsigned char *version, int len);

#ifdef cplusplus
	extern "C" {
#endif
