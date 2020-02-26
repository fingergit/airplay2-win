

#ifndef _XDW_SOCKET_IPC_H_
#define _XDW_SOCKET_IPC_H_

#include <stdio.h>
#include <stdlib.h>
//#include <errno.h>
//#include <string.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/un.h>
//#include <sys/times.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <sys/select.h>
//#include <netinet/in.h>
#include "xdw_list.h"
#include "threads.h"

typedef enum _fn_xdw_msg_q_name_e_
{
	fn_msg_q_main = 0,
	fn_msg_q_dlna,
	fn_msg_q_airplay,
	fn_msg_q_private,
	fn_msg_q_player,
	fn_msg_q_airplay_music,
	fn_msg_q_max
}fn_xdw_msg_q_name_e;


typedef struct _fn_xdw_msg_t_
{
	struct xdw_q_head msg_q[fn_msg_q_max];
#if 0
	pthread_cond_t has_node[fn_msg_q_max];
#else
    event_handle_t has_node[fn_msg_q_max];
#endif
}fn_xdw_msg_t;

typedef struct _fn_xdw_msg_node_t_
{
	unsigned long msg;
	XDW_64 tm;
	struct xdw_list_head list;
}fn_xdw_msg_node_t;


#if 0
typedef enum _fn_thread_name_e_
{
	fn_thread_read = 0,
	fn_thread_decode,
	fn_thread_audio,
	fn_thread_main,
	fn_thread_max
}fn_thread_name_e;


typedef struct _fn_threads_info_t_
{
	XDW_64 total_time[fn_thread_max];
	XDW_64 last_enter[fn_thread_max];
	XDW_64 last_exit[fn_thread_max];
}fn_threads_info_t;
#endif

void*   xdw_socket_get_msg_q(fn_xdw_msg_q_name_e a_msgname);
int		xdw_socket_create(void);
int		xdw_socket_wait(fn_xdw_msg_q_name_e fd, int *msg, XDW_64 *tm, int no_dealy);
int		xdw_socket_post(int msg,fn_xdw_msg_q_name_e peer_fd);
void	xdw_socket_clear(fn_xdw_msg_q_name_e fd);
void	xdw_socket_destroy(void);

#if 0
void fn_threads_info_init(void);
void fn_thread_func_enter(fn_thread_name_e which);
void fn_thread_func_exit(fn_thread_name_e which);
void fn_thread_cpu_time_print(void);

int xdw_socket_xdw_create(unsigned short port,int non_blocked);
void xdw_socket_xdw_destroy(int fd);
int xdw_socket_xdw_wait(int fd,int *msg);
int xdw_socket_xdw_post(int fd,int msg,unsigned short peer_port);
void xdw_socket_xdw_clear(int fd);
int xdw_socket_xdw_wait_nonblock(int fd,int *msg);
#endif

#endif //_XDW_SOCKET_IPC_H_

