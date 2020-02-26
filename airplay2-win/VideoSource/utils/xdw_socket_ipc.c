#include "xdw_socket_ipc.h"
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <stdint.h>
#define xdw_free free
#define xdw_malloc malloc
#define  XDW_DBG_LOG(...)  printf
//#define xdw_get_sys_time_ms av_gettime


static fn_xdw_msg_t fn_xdw_msg_q;

int64_t xdw_get_sys_time_ms(void)
{
	//struct timeval tv;
	//gettimeofday(&tv, NULL);
	//return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;

	struct timeval tp;

    FILETIME ft;
	long long t;
	// Windows CE does not define GetSystemTimeAsFileTime so we do it in two steps.
	SYSTEMTIME st;
	GetSystemTime( &st );
	SystemTimeToFileTime( &st, &ft );
	t = (((long long)(ft.dwHighDateTime)) << 32) | ft.dwLowDateTime;
	#if !defined( BOOST_MSVC ) || BOOST_MSVC > 1300 // > VC++ 7.0
	t -= 116444736000000000LL;
	#else
	t -= 116444736000000000;
	#endif
	t /= 10;  // microseconds
	tp.tv_sec =  (long)( t / 1000000UL);
	tp.tv_usec = (long)( t % 1000000UL);

	return (int64_t)tp.tv_sec * 1000000 + tp.tv_usec;
}


int xdw_socket_create(void)
{
	int i;
	//XDW_DBG_LOG("ipc: create called...\n");
	for(i = 0; i < fn_msg_q_max; i++)
	{
		xdw_q_init(&fn_xdw_msg_q.msg_q[i],fn_q_mode_block_empty);
	}	
	return 0;
}
void* xdw_socket_get_msg_q(fn_xdw_msg_q_name_e a_msgname)
{
	return (void*) &fn_xdw_msg_q.msg_q[a_msgname];
}

int xdw_socket_wait(fn_xdw_msg_q_name_e fd, int *msg, XDW_64 *tm, int no_dealy)
{
	int ret;
	struct xdw_list_head *ptr;
	fn_xdw_msg_node_t *msg_node;



	ptr = xdw_q_pop(&fn_xdw_msg_q.msg_q[fd]);
	


	if(!ptr)
	{
//		XDW_DBG_LOG(" q already empty, fd = %d,do not blocked!\n",fd);
		return -1;
	}
	else
	{
		msg_node = (fn_xdw_msg_node_t *)list_entry(ptr, fn_xdw_msg_node_t, list);
		*msg = msg_node->msg;
		*tm = msg_node->tm;
		xdw_free(msg_node);
		return 1;
	}
}

int xdw_socket_post(int msg,fn_xdw_msg_q_name_e peer_fd)
{
	int ret;
	fn_xdw_msg_node_t *msg_node;
	msg_node = xdw_malloc(sizeof(fn_xdw_msg_node_t));
	if(!msg_node)
	{
		XDW_DBG_LOG("ipc: alloc msg_node err!\n");
		return -1;
	}
	//allocate OK, add to ipc q
//	XDW_DBG_LOG("ipc: xdw_socket_xdw_post msg = [%d]\n",msg);
	msg_node->msg = msg;
	msg_node->tm = xdw_get_sys_time_ms();
	xdw_q_push(&msg_node->list, &fn_xdw_msg_q.msg_q[peer_fd]);
	return 0;
}

void xdw_socket_clear(fn_xdw_msg_q_name_e fd)
{
	int msg;
	fn_xdw_msg_node_t *msg_node;
	struct xdw_list_head *poped_item;
	struct xdw_q_head *xdw_q_head = &fn_xdw_msg_q.msg_q[fd];
	xdw_mutex_lock(xdw_q_head->mutex);
	//XDW_DBG_LOG("clearing fd %d\n",fd);
	while(!xdw_list_empty(&xdw_q_head->lst_head))
	{
		poped_item = xdw_q_head->lst_head.next;
		xdw_list_del(poped_item);
		xdw_q_head->node_nr--;
		msg_node = (fn_xdw_msg_node_t *)list_entry(poped_item, fn_xdw_msg_node_t, list);
		XDW_DBG_LOG("skipping msg %d\n", msg_node->msg);
		xdw_free(msg_node);
	}
	xdw_mutex_unlock(xdw_q_head->mutex);

	return;	
}


void xdw_socket_destroy(void)
{
	int i;
	XDW_DBG_LOG("ipc: destroy called...\n");
	for(i = 0; i < fn_msg_q_max; i++)
	{
		xdw_socket_clear(i);
		xdw_q_destroy(&fn_xdw_msg_q.msg_q[i]);
	}
}

