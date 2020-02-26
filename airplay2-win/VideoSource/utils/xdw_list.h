
#ifndef _XDW_LIST_H__
#define _XDW_LIST_H__

#include "threads.h"
#include "xdw_std.h"
#include "xdw_lock.h"


#ifdef __cplusplus
extern "C"
{
#endif
//#include "list.h"

struct xdw_list_head 
{
	struct xdw_list_head *next, *prev;
};
 
#define XDW_LIST_HEAD_INIT(name) { &(name), &(name) }
 
#define XDW_LIST_HEAD(name) \
		   struct xdw_list_head name = XDW_LIST_HEAD_INIT(name)
 

//list_entry(ptr,xdw_ffmpeg_packet_qnode_t,list)
//(xdw_ffmpeg_packet_qnode_t *)((char *)(ptr) - (unsigned long)(&((xdw_ffmpeg_packet_qnode_t *)0)->list)))

#define list_entry(ptr, type, member) \
   ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_for_each_entry(pos, head, member)                \
	for (pos = list_entry((head)->next, typeof(*pos), member);    \
		&pos->member != (head);     \
		pos = list_entry(pos->member.next, typeof(*pos), member))


void xdw_list_del(struct xdw_list_head *entry);
int  xdw_list_empty(const struct xdw_list_head *head);
void xdw_list_add(struct xdw_list_head *newly, struct xdw_list_head *head);
void xdw_xdw_list_add_tail(struct xdw_list_head *newly, struct xdw_list_head *head);

////////////////////////// q  API ///////////////////////////////////////////

typedef enum _q_node_mode_e_
{
	fn_q_mode_unblock = 0x0,
	fn_q_mode_block_empty = 0x1,
	fn_q_mode_block_full = 0x2,	
	fn_q_mode_max
}q_node_mode_e;


typedef enum _q_node_property_e_
{
	fn_q_node_max_nr = 0,
	fn_q_node_empty_func,
	fn_q_node_full_func,
	fn_q_node_push_func,
	fn_q_node_pop_func,
	fn_q_node_priv_data,
	fn_q_node_max
}q_node_property_e;

typedef  int (* fn_q_check_full_cb_t)(void *);
typedef  int (* fn_q_check_empty_cb_t)(void *);
typedef void (* fn_q_full_cb_t)(void *);
typedef void (* fn_q_empty_cb_t)(void *);
typedef void (* fn_xdw_q_push_cb_t)(void *,struct xdw_list_head *newly);
typedef void (* fn_xdw_q_pop_cb_t)(void *,struct xdw_list_head *poped);


struct xdw_q_head
{
	struct xdw_list_head lst_head;
	void *mutex;  // q mutex lock
	int node_nr;			// node count
	int max_node_nr;		// upper limit node count
	q_node_mode_e	mode;	// once this mode value is set, you should never change it during the q's life
	q_node_mode_e	origin_mode;
#if 0
	pthread_cond_t  has_node;  // if empty, blocked
	pthread_cond_t  has_space;  // if full, blocked
#else
	event_handle_t  has_node;
  event_handle_t  has_space;
#endif
	fn_q_check_full_cb_t is_full_func;
	fn_q_check_empty_cb_t is_empty_func;
	fn_q_full_cb_t full_func;
	fn_q_empty_cb_t empty_func;
	fn_xdw_q_push_cb_t  push_func;
	fn_xdw_q_pop_cb_t	pop_func;
	void *empty_parm;
	void *full_parm;
	int stop_blocking_empty;
	int stop_blocking_full;
	XDW_64 data_size;
	void *priv;
	int cal_pts_denominator;
	int need_reset_time_limit;
	XDW_64 start_node_pts;
	XDW_64 current_node_pts;
};

void xdw_q_set_property(struct xdw_q_head *xdw_q_head,q_node_property_e property,void *parm1,void *parm2);
int xdw_q_init(struct xdw_q_head *xdw_q_head,q_node_mode_e mode);
void xdw_q_destroy(struct xdw_q_head *xdw_q_head);
int xdw_q_push(struct xdw_list_head *newly,struct xdw_q_head *xdw_q_head);
int xdw_q_is_empty(struct xdw_q_head *xdw_q_head);
struct xdw_list_head *xdw_q_get_first(struct xdw_q_head *xdw_q_head);
struct xdw_list_head *xdw_q_pop(struct xdw_q_head *xdw_q_head);
int xdw_q_get_max_node_nr(struct xdw_q_head *xdw_q_head);
int xdw_q_get_node_nr(struct xdw_q_head *xdw_q_head);

#ifdef __cplusplus
}
#endif

#endif //__XDW_LIST_H__
