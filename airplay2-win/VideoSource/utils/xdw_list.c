
#include "xdw_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define  XDW_DBG_LOG(...)  printf


static  void XDW_INIT_LIST_HEAD(struct xdw_list_head *list)
{
	list->next = list;
	list->prev = list;
}

static  void __xdw_list_add(struct xdw_list_head *newly,
								 struct xdw_list_head *prev,
								 struct xdw_list_head *next)
{
	next->prev = newly;
	newly->next = next;
	newly->prev = prev;
	prev->next = newly;
}

static  void __xdw_list_del(struct xdw_list_head * prev, struct xdw_list_head * next)
{
	next->prev = prev;
	prev->next = next;
}


  void xdw_list_add(struct xdw_list_head *newly, struct xdw_list_head *head)
{
	__xdw_list_add(newly, head, head->next);
}
 

  void xdw_xdw_list_add_tail(struct xdw_list_head *newly, struct xdw_list_head *head)
{
	__xdw_list_add(newly, head->prev, head);
}

  void xdw_list_del(struct xdw_list_head *entry)
{
	__xdw_list_del(entry->prev, entry->next);
}


  int xdw_list_empty(const struct xdw_list_head *head)
{
	return (head->next == head);
}

////////////////////////////////////// q API ///////////////////////////////////////

  void xdw_q_set_property(struct xdw_q_head *xdw_q_head,q_node_property_e property,void *parm1,void *parm2)
{
	xdw_mutex_lock(xdw_q_head->mutex);
	switch(property)
	{
		case fn_q_node_max_nr:
		{
			xdw_q_head->max_node_nr = (int)parm1;
			break;
		}
		case fn_q_node_empty_func:
		{
			xdw_q_head->empty_func = (fn_q_empty_cb_t)parm1;
			xdw_q_head->empty_parm = parm2;
			break;
		}
		case fn_q_node_full_func:
		{
			xdw_q_head->full_func = (fn_q_full_cb_t)parm1;
			xdw_q_head->full_parm = parm2;
			break;
		}
		case fn_q_node_push_func:
		{
			xdw_q_head->push_func = (fn_xdw_q_push_cb_t)parm1;
			break;
		}
		case fn_q_node_pop_func:
		{
			xdw_q_head->pop_func = (fn_xdw_q_pop_cb_t)parm1;
			break;
		}
		case fn_q_node_priv_data:
		{
			xdw_q_head->priv = parm1;
			break;
		}
		default:
			break;
	}
	xdw_mutex_unlock(xdw_q_head->mutex);
	return;
}

static  int xdw_q_default_is_full_func(void *param)
{
	struct xdw_q_head *xdw_q_head = (struct xdw_q_head *)(param);
	return (xdw_q_head->node_nr >= xdw_q_head->max_node_nr);
}

static  int xdw_q_default_is_empty_func(void *param)
{
	struct xdw_q_head *xdw_q_head = (struct xdw_q_head *)(param);
	return (!xdw_q_head->node_nr);
}

void reset_q_init(struct xdw_q_head *old_head, struct xdw_q_head *head)
{   
  head->mode	  = old_head->mode;
  head->has_node  = old_head->has_node;
  head->has_space = old_head->has_space;  

  if(old_head->mode & fn_q_mode_block_empty)
  {
	  //pthread_cond_destroy(&old_head->has_node); 
	  //pthread_cond_init(&head->has_node,(const pthread_condattr_t *)NULL); 

	  EVENT_DESTORY(old_head->has_node);
	  EVENT_CREATE(head->has_node);
  }
  
  if(old_head->mode & fn_q_mode_block_full)
  {
	 // pthread_cond_destroy(&old_head->has_space);
	 // pthread_cond_init(&head->has_space,(const pthread_condattr_t *)NULL);

	  EVENT_DESTORY(old_head->has_space);
	  EVENT_CREATE(head->has_space);
  } 	  


  if( head->node_nr != 0 )
  {
	  head->lst_head.next->prev = &head->lst_head;
	  head->lst_head.prev->next = &head->lst_head;
  }
  else
  {
	  head->lst_head.next = &head->lst_head;
	  head->lst_head.prev = &head->lst_head;
  }

}
  int xdw_q_init(struct xdw_q_head *xdw_q_head,q_node_mode_e mode)
{
//	xdw_memset(xdw_q_head,0,sizeof(struct xdw_q_head));
	XDW_INIT_LIST_HEAD(&xdw_q_head->lst_head);
	xdw_q_head->node_nr = 0;
	xdw_q_head->mutex = xdw_mutex_init();
	if(!xdw_q_head->mutex)
	{
		return -1;
	}

	xdw_q_head->mode = mode;
	xdw_q_head->origin_mode = mode;

	if(mode & fn_q_mode_block_empty)
	{
		//pthread_cond_init(&xdw_q_head->has_node,(const pthread_condattr_t *)NULL); 

		EVENT_CREATE(xdw_q_head->has_node);
	}

	if(mode & fn_q_mode_block_full)
	{
		//pthread_cond_init(&xdw_q_head->has_space,(const pthread_condattr_t *)NULL);
		EVENT_CREATE(xdw_q_head->has_space);
	}

	xdw_q_head->max_node_nr = 0xFFFF; // default value
	xdw_q_head->is_full_func = xdw_q_default_is_full_func;
	xdw_q_head->is_empty_func = xdw_q_default_is_empty_func;
	
	return 0;
}

  void xdw_q_destroy(struct xdw_q_head *xdw_q_head)
{
	if(xdw_q_head->mode & fn_q_mode_block_empty)
	{
		//pthread_cond_destroy(&xdw_q_head->has_node); 

		EVENT_DESTORY(xdw_q_head->has_node);
	}

	if(xdw_q_head->mode & fn_q_mode_block_full)
	{
		//pthread_cond_destroy(&xdw_q_head->has_space);
		EVENT_DESTORY(xdw_q_head->has_space);
	}

	xdw_mutex_uninit(xdw_q_head->mutex);
}

//
// @ return value: 
// 0 : item is pushed
// -1:  q full , item is not pushed
//
 int xdw_q_push(struct xdw_list_head *newly,struct xdw_q_head *xdw_q_head)
{
	//xdw_mutex_lock(xdw_q_head->mutex);
	if(xdw_q_head->mode & fn_q_mode_block_full)
	{
		//while(xdw_q_head->node_nr >= xdw_q_head->max_node_nr)
		while(xdw_q_head->is_full_func(xdw_q_head))
		{
			// block until newly node added
			if(xdw_q_head->full_func)
			{
				//xdw_mutex_unlock(xdw_q_head->mutex);
				xdw_q_head->full_func(xdw_q_head->full_parm);
			}

			xdw_mutex_lock(xdw_q_head->mutex);
			//pthread_cond_wait(&xdw_q_head->has_space,(pthread_mutex_t*)xdw_q_head->mutex);
			EVENT_WAIT(xdw_q_head->has_space);
			xdw_mutex_unlock(xdw_q_head->mutex);
			
			if(xdw_q_head->stop_blocking_full)
			{
//				XDW_DBG_LOG("stop blocking full...\n");
				break;
			}
		}
	}
	else
	{
		//if(xdw_q_head->node_nr >= xdw_q_head->max_node_nr)
		if(xdw_q_head->is_full_func(xdw_q_head))
		{
			if(xdw_q_head->full_func)
			{
				xdw_q_head->full_func(xdw_q_head->full_parm);
			}
		}
	}

	xdw_mutex_lock(xdw_q_head->mutex);
	xdw_xdw_list_add_tail(newly,&xdw_q_head->lst_head);
	xdw_q_head->node_nr++;
//	XDW_DBG_LOG("e:xdw_q_head = %x,xdw_q_head->node_nr = %d\n",xdw_q_head,xdw_q_head->node_nr);
	xdw_mutex_unlock(xdw_q_head->mutex);

	if(xdw_q_head->mode & fn_q_mode_block_empty)
	{

	//pthread_cond_signal(&xdw_q_head->has_node);
	EVENT_POST(xdw_q_head->has_node);

	}
	if(xdw_q_head->push_func)
	{

		xdw_q_head->push_func(xdw_q_head,newly);	
	}
	return 0;
}

  int xdw_q_is_empty(struct xdw_q_head *xdw_q_head)
{
	return (!xdw_q_head->node_nr);
}

  int xdw_q_get_node_nr(struct xdw_q_head *xdw_q_head)
{
	return xdw_q_head->node_nr;
}

  int xdw_q_get_max_node_nr(struct xdw_q_head *xdw_q_head)
{
	return xdw_q_head->max_node_nr;
}


// if already empty, would block if mode is block, otherwise returns NULL
struct xdw_list_head *xdw_q_pop(struct xdw_q_head *xdw_q_head)
{
	struct xdw_list_head *poped_item;
	xdw_mutex_lock(xdw_q_head->mutex);

	if(xdw_q_head->mode & fn_q_mode_block_empty)
	{
		// block mode
		//while(xdw_list_empty(&xdw_q_head->lst_head))
		while(xdw_q_head->is_empty_func(xdw_q_head))
		{
			// block until newly code added
			if(xdw_q_head->empty_func)
			{
//				XDW_DBG_LOG("pop empty wait, calling empty func\n");
				xdw_mutex_unlock(xdw_q_head->mutex);
				xdw_q_head->empty_func(xdw_q_head->empty_parm);
				xdw_mutex_lock(xdw_q_head->mutex);
			}
			//pthread_cond_wait(&xdw_q_head->has_node,(pthread_mutex_t*)xdw_q_head->mutex);
			{
				xdw_mutex_unlock(xdw_q_head->mutex);
				EVENT_WAIT(xdw_q_head->has_node);
				xdw_mutex_lock(xdw_q_head->mutex);

			}
//			XDW_DBG_LOG("stop pthread_cond_wait\n");
			if(xdw_q_head->stop_blocking_empty)
			{
	//			XDW_DBG_LOG("stop blocking empty...\n");
				break;
			}
		}
	}
	else
	{
		// unblock mode
		//if(xdw_list_empty(&xdw_q_head->lst_head))
		if(xdw_q_head->is_empty_func(xdw_q_head))
		{
			// unblock mode
			xdw_mutex_unlock(xdw_q_head->mutex);
			if(xdw_q_head->empty_func)
			{
				xdw_q_head->empty_func(xdw_q_head->empty_parm);
			}
			return (struct xdw_list_head *)NULL;
		}
	}
	
	poped_item = xdw_q_head->lst_head.next;
	xdw_list_del(poped_item);
	xdw_q_head->node_nr--;
	xdw_mutex_unlock(xdw_q_head->mutex);
	if(xdw_q_head->mode & fn_q_mode_block_full)
	{

		//pthread_cond_signal(&xdw_q_head->has_space);
		EVENT_POST(xdw_q_head->has_space);
	}
	if(xdw_q_head->pop_func)
	{
		xdw_q_head->pop_func(xdw_q_head,poped_item);	
	}
	return poped_item;
}



// if already empty, returns NULL
// get first element
  struct xdw_list_head *xdw_q_get_first(struct xdw_q_head *xdw_q_head)
{
	struct xdw_list_head *got_item;
	xdw_mutex_lock(xdw_q_head->mutex);
	if(xdw_list_empty(&xdw_q_head->lst_head))
	{
		xdw_mutex_unlock(xdw_q_head->mutex);
		return (struct xdw_list_head *)NULL;
	}
	got_item = xdw_q_head->lst_head.next;
	xdw_mutex_unlock(xdw_q_head->mutex);
	return got_item;
}
  struct list_head *q_get_last(struct xdw_q_head *xdw_q_head)
{
	struct list_head *got_item;
	xdw_mutex_lock(xdw_q_head->mutex);
	if (xdw_list_empty(&xdw_q_head->lst_head))
	{
		xdw_mutex_unlock(xdw_q_head->mutex);
		return (struct list_head *)NULL;
	}
	got_item = xdw_q_head->lst_head.prev;
	xdw_mutex_unlock(xdw_q_head->mutex);
	return got_item;
}
