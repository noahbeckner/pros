/**
 * pthread.c - Port of pthreads to PROS for V5
 *
 * Contains the various methods needed to enable pthreads support
 * based on the espressif/esp-idf repository for use with libcxxrt.
 *
 * Espressif IoT Development Framework is licensed under the Apache 2.0 license
 *
 * Copyright (c) 2017, Purdue University ACM SIGBots.
 * All rights reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _POSIX_THREADS
#define _POSIX_THREADS
#endif

#ifndef _UNIX98_THREAD_MUTEX_ATTRIBUTES
#define _UNIX98_THREAD_MUTEX_ATTRIBUTES
#endif

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
/*#include "kapi.h"*/
#include "rtos/FreeRTOS.h"
#include "rtos/list.h"
#include "rtos/semphr.h"
#include "rtos/task.h"
#include "pros/llemu.h"
//#include "system/system.h"

#define RTOS_PTHREAD_STATE_EXITED       (1 << 1)
#define RTOS_PTHREAD_STATE_RUN          (1 << 2)
#define CURRENT_TASK NULL
#define TASK_STACK_DEPTH_DEFAULT 0x2000
#define TASK_STACK_DEPTH_MIN 0x200
#define TASK_PRIORITY_DEFAULT 8
#define TASK_PRIORITY_MAX 16

struct rtos_pthread {
  list_item_t list_item; task_t  join_handle; uint16_t state; uint8_t detached;
  void *retval;
};

struct rtos_pthread_task_arg {
  void* (*fn)(void*);
  void* arg;
};

struct rtos_pthread_mutex {
  list_item_t list_item;
  sem_t sem;
  int type;
};

static sem_t threads_mut = NULL;
static static_sem_s_t threads_mut_buf;

static List_t threads_list;

void debug(int code, const char* msg) {
  lcd_print(code, msg);
  task_delay(1000);
}

void rtos_pthread_init() {
  vListInitialise(&threads_list);
  threads_mut = mutex_create_static(&threads_mut_buf);
}

static void* rtos_pthread_find_item(void* (*item_check_fn)(list_item_t*, void*), void* check_arg) {
  list_item_t const * list_end = listGET_END_MARKER(&threads_list);
  list_item_t* list_item = listGET_HEAD_ENTRY(&threads_list);
  while(list_item != list_end) {
    void* val = item_check_fn(list_item, check_arg);
    if(val) {
      return val;
    }
    list_item = listGET_NEXT(list_item);
  }
  return NULL;
}

static void* rtos_pthread_get_handle_by_desc(list_item_t* item, void* arg) {
  struct rtos_pthread* pthread = (struct rtos_pthread*)listGET_LIST_ITEM_OWNER(item);
  if(pthread == arg) {
    return (void*)listGET_LIST_ITEM_VALUE(item);
  }
  return NULL;
}

static task_t rtos_pthread_find_handle(pthread_t thread) {
  return (task_t)rtos_pthread_find_item(rtos_pthread_get_handle_by_desc, (void*)thread);
}

static void* rtos_pthread_get_desc_by_handle(list_item_t* item, void* arg) {
  task_t task_handle = arg;
  task_t comp_handle = (task_t)(listGET_LIST_ITEM_VALUE(item));
  if(task_handle == comp_handle) {
    return (struct rtos_pthread*)listGET_LIST_ITEM_OWNER(item);
  }
  return NULL;
}

static struct rtos_pthread* rtos_pthread_find(task_t task_handle) {
  return (struct rtos_pthread*)rtos_pthread_find_item(rtos_pthread_get_desc_by_handle, (void*)task_handle);
}

static void rtos_pthread_delete(struct rtos_pthread* pthread) {
  uxListRemove(&pthread->list_item);
  kfree(pthread);
}

/********************Core Functions********************/
void pthread_task_fn(void* _arg) {
  struct rtos_pthread_task_arg* arg = (struct rtos_pthread_task_arg*)_arg;
  void* rval = NULL;
  //wait for parent pthread_create to finish it's stuff....
  task_notify_wait(0, 0, NULL, portMAX_DELAY);

  rval = arg->fn(arg->arg);

  kfree(arg);
  
  pthread_exit(rval); //stubbed retval for now
}

int pthread_create(pthread_t* thread, pthread_attr_t const * attr,
                   void* (start_routine)(void*), void* arg) {
  if(attr) {
    return ENOSYS;
  }

  struct rtos_pthread_task_arg* task_arg = (struct rtos_pthread_task_arg*)kmalloc(sizeof(task_arg));
  if(task_arg == NULL) {
    return ENOMEM;
  }
  memset(task_arg, 0, sizeof(*task_arg));

  struct rtos_pthread* pthread = (struct rtos_pthread*)kmalloc(sizeof(*pthread));
  if(pthread == NULL) {
    kfree(task_arg);
    return ENOMEM;
  }
  memset(pthread, 0, sizeof(*pthread));

  task_arg->fn = (void* (*)(void*))start_routine;
  task_arg->arg = arg;

  uint32_t stack_size = TASK_STACK_DEPTH_DEFAULT;
  uint32_t prio = TASK_PRIORITY_DEFAULT;

  if(attr) {
    stack_size = attr->stacksize; 
     
    switch(attr->detachstate) {
      case PTHREAD_CREATE_DETACHED:
        pthread->detached = true;
        break;
      case PTHREAD_CREATE_JOINABLE:
      default:
        pthread->detached = false;
    }
  }

  task_t task = task_create(&pthread_task_fn, task_arg, prio, stack_size, "pthread");
  if(task == NULL) {
    kfree(pthread);
    kfree(task_arg);
    return EAGAIN;
  }

  vListInitialiseItem(&pthread->list_item);
  listSET_LIST_ITEM_OWNER(&pthread->list_item, pthread);
  listSET_LIST_ITEM_VALUE(&pthread->list_item, (uint32_t)task);

	if(threads_mut == NULL) {
		lcd_print(2,"Error!");
		task_delay(1000);
	}
  if(sem_wait(threads_mut, portMAX_DELAY) != pdTRUE) {
    kfree(pthread);
    kfree(task_arg);
    task_delete(task);
    return EAGAIN;
  }

  vListInsertEnd(&threads_list, &pthread->list_item);

  sem_post(threads_mut);

  //notify task to break wait in pthread_task_fn
  task_notify_ext(task, 0, E_NOTIFY_ACTION_NONE, NULL);
  //Set pthread run state
  pthread->state = RTOS_PTHREAD_STATE_RUN; 

  *thread = (pthread_t)pthread;
  return 0;
}

int pthread_join(pthread_t thread, void** retval) {
  struct rtos_pthread* pthread = (struct rtos_pthread*)thread;
  int ret = 0;
  bool wait = false;
  void* task_retval = NULL;

  if(sem_wait(threads_mut, portMAX_DELAY) != pdTRUE) {
    ret = EAGAIN;
  }
  task_t task = rtos_pthread_find_handle(thread);
  if(!task) {
    //task not found
    ret = ESRCH;
  } else if(pthread->join_handle) {
    //thread already set to join another thread
    ret = EINVAL;
  } else if(pthread->detached) {
    //if thread is already detached
    ret = EINVAL;
  }
  else if(task == task_get_current()) {
    //can't join self
    ret = EDEADLK;
  } else {
    struct rtos_pthread* cur_pthread = rtos_pthread_find(task_get_current());
    if(cur_pthread && cur_pthread->join_handle == task) {
      //join to each other is pretty dumb
      ret = EDEADLK;
    } else {
      if(pthread->state == RTOS_PTHREAD_STATE_RUN) {
        //pthread is currently running
        pthread->join_handle = task_get_current();
        wait = true;
      } else {
        task_retval = pthread->retval;
        rtos_pthread_delete(pthread);
      }
    }
  }
  sem_post(threads_mut);

  if(ret == 0) {
    //blocks this task until joining task sends a notification
    //see: pthread_task_fn
    if(wait) {
      task_notify_wait(0, 0, NULL, portMAX_DELAY);
      if(sem_wait(threads_mut, portMAX_DELAY) != pdTRUE) {
        errno = ENOMSG;
        return ENOMSG;
      }
      task_retval = pthread->retval;
      rtos_pthread_delete(pthread);
      sem_post(threads_mut);
    }
    task_delete(task);
  }

  if(retval) {
    *retval = task_retval;
  }

  return ret;
} 

int pthread_detach(pthread_t thread) { 
  struct rtos_pthread* pthread = (struct rtos_pthread*) &thread; 
  int ret = 0;
  if(sem_wait(threads_mut, portMAX_DELAY) != pdTRUE) {
    errno = ENOMSG;
    return EAGAIN;
  }

  task_t task = rtos_pthread_find_handle(thread);
  if(!task) {
    errno = ESRCH;
    ret = ESRCH;
  } 
  else if(pthread->detached) {
    //already detached
    errno = EINVAL;
    ret = EINVAL;
  }
  else if(pthread->join_handle) {
    //already waiting to join some other thread
    errno = EINVAL;
    ret = EINVAL;
  }
  else if(pthread->state == RTOS_PTHREAD_STATE_RUN) {
    //if thread is currently running
    pthread->detached = true;
  }
  else {
    //if thread has already finished running, release sys-resource 
    rtos_pthread_delete(pthread);
    task_delete(task);
  }
  sem_post(threads_mut);
  return ret;
}

void pthread_exit(void* ret_val) {
  bool detached = false; 

  if(sem_wait(threads_mut, portMAX_DELAY) != pdTRUE) {
    errno = ENOMSG;
  }
  else {  //require this wierdness because func is noreturn
    struct rtos_pthread* pthread = rtos_pthread_find(task_get_current());
    bool valid = false;
    if(!pthread) {
      errno = ESRCH;
    }
    else if(pthread->state == RTOS_PTHREAD_STATE_EXITED) {
      errno = EINVAL;
    }
    else if(pthread->detached) {
      //if pthread is detached
      printf("Exited detached thread properly\n");
      rtos_pthread_delete(pthread);
      detached = true;
      valid = true;
    }
    else {
      //if pthread is not detached
      //retval stuff goes here eventually
      pthread->retval = ret_val;
      valid = true; 
      if(pthread->join_handle) {
        //if a join_handle exists
        //notify to join
        task_notify_ext(pthread->join_handle, 0, E_NOTIFY_ACTION_NONE, NULL);
      }
      else {
        pthread->state = RTOS_PTHREAD_STATE_EXITED;
      }
    }
    sem_post(threads_mut);
    
    if(valid) {
      if(detached) {
        task_delete(NULL);  //NULL deletes current task
      }
      else {
        //If task isn't detached, suspend until join cleans
        task_suspend(NULL);
      }

      printf("If the program reaches this, something broke horribly\n");
    }
  }
  exit(1);
}
/*******************************************/

/******************Utility******************/
int phread_cancel(pthread_t pthread) {
  errno = ENOSYS;
  return ENOSYS;
}

pthread_t pthread_self() {
  if(!sem_wait(threads_mut, portMAX_DELAY)) {
    errno = EAGAIN;
    return EAGAIN;
  }
  struct rtos_pthread* pthread = rtos_pthread_find(task_get_current());
  sem_post(threads_mut);
  if(!pthread) {
    errno = EINVAL;
  }
  return (pthread_t)pthread;
}

int pthread_equal(pthread_t t1, pthread_t t2) {
  return t1 == t2 ? 1 : 0;
}

int sched_yield() {
  task_delay(0);
  return 0;
}
/*******************************************/

/******************Attributes******************/
int pthread_attr_init(pthread_attr_t* attr) {
  //Set suppported fields to default
  if(attr) {
    attr->stacksize = TASK_STACK_DEPTH_DEFAULT;
    attr->detachstate = PTHREAD_CREATE_JOINABLE;    
    return 0;
  }
  return EINVAL;
}

int pthread_attr_destroy(pthread_attr_t* attr) {
  //Nothing to deallocate or destroy. Restore fiels to defaults
  return pthread_attr_init(attr);
}

int pthread_attr_getstacksize(const pthread_attr_t* attr, size_t* stacksize) {
  if(attr) {
    *stacksize = attr->stacksize;
    return 0;
  }
  return EINVAL;
}

int pthread_attr_setstacksize(pthread_attr_t* attr, size_t stacksize) {
  if(attr && !(stacksize < TASK_STACK_DEPTH_MIN)) {
    attr->stacksize = stacksize;
    return 0;
  }
  return EINVAL;
}

int pthread_attr_getdetachstate(const pthread_attr_t* attr, int* detachstate) {
  if(attr) {
    *detachstate = attr->detachstate;
    return 0;
  }
  return EINVAL;
}

int pthread_attr_setdetachstate(pthread_attr_t* attr, int detachstate) {
  if(attr) {
    switch(detachstate) {
      case PTHREAD_CREATE_JOINABLE:
        attr->detachstate = PTHREAD_CREATE_JOINABLE;
        break;
      case PTHREAD_CREATE_DETACHED:
        attr->detachstate = PTHREAD_CREATE_DETACHED;
        break;
      default:
        return EINVAL;
    }
    return 0;
  }
  return EINVAL;
}
/****************************************/

/***************Scheduling**************/
//Needs testing....
int pthread_attr_getschedparam(const pthread_attr_t * attr, struct sched_param * param) {
  int ret = 0;

  if(!attr || !param) ret = EINVAL;
  else {
    param->sched_priority = (attr->schedparam).sched_priority;
  }
  return ret;
}

int pthread_attr_setschedparam(pthread_attr_t* attr, const struct sched_param* param) {
  int ret = 0;
  /* Check for NULL param. */
  if(!attr || !param) {
      ret = EINVAL;
  }
  else {
    /* Ensure that param.sched_priority is valid. */
    if((param->sched_priority > TASK_PRIORITY_MAX) || (param->sched_priority < 0)) {
      ret = EINVAL;
    }
    else {
      (attr->schedparam).sched_priority = param->sched_priority;
    }
  }
  return ret;
}

//Stubbed out, because I don't know what to do with this
int pthread_attr_setschedpolicy(pthread_attr_t * attr, int policy) {
  /* Silence warnings about unused parameters. */
  (void) attr;
  (void) policy;

  return 0;
}

int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param) {
  int ret = 0;
  /* Silence compiler warnings about unused parameters. */
  (void) policy;

  task_t task = rtos_pthread_find_handle(thread);
  /* Change the priority of the FreeRTOS task. */
  task_set_priority(task, param->sched_priority);

  return ret;
}

int pthread_getschedparam(pthread_t thread, int* policy, struct sched_param * param) {
  int ret = 0;
  /* Silence compiler warnings about unused parameters. */
  (void) policy;

  task_t task = rtos_pthread_find_handle(thread);
  /* Change the priority of the FreeRTOS task. */
  param->sched_priority = task_get_priority(task);
  return ret;
}

/**************************************/

/******************Mutex******************/
static int mutexattr_check(const pthread_mutexattr_t *attr)
{
    if (attr->type < PTHREAD_MUTEX_NORMAL || attr->type > PTHREAD_MUTEX_RECURSIVE) {
        return EINVAL;
    }
    return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    int type = PTHREAD_MUTEX_NORMAL;

    if (!mutex) {
        return EINVAL;
    }

    if (attr) {
        if (!attr->is_initialized) {
            return EINVAL;
        }
        int res = mutexattr_check(attr);
        if (res) {
            return res;
        }
        type = attr->type;
    }

    struct rtos_pthread_mutex *mux = (struct rtos_pthread_mutex*)kmalloc(sizeof(struct rtos_pthread_mutex));
    if (!mux) {
        return ENOMEM;
    }
    mux->type = type;

    if (mux->type == PTHREAD_MUTEX_RECURSIVE) {
        mux->sem = mutex_recursive_create();
    } else {
        mux->sem = mutex_create();
    }
    if (!mux->sem) {
        kfree(mux);
        return EAGAIN;
    }

    *mutex = (pthread_mutex_t)mux; // pointer value fit into pthread_mutex_t (uint32_t)

    return 0;
}

static int pthread_mutex_lock_internal(struct rtos_pthread_mutex *mux, uint32_t tmo)
{
    if (mux->type == PTHREAD_MUTEX_RECURSIVE) {
        if (mutex_recursive_take(mux->sem, tmo) != pdTRUE) {
            return EBUSY;
        }
    } else {
        if (sem_wait(mux->sem, tmo) != pdTRUE) {
            return EBUSY;
        }
    }

    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if (!mutex) {
        return EINVAL;
    }
    struct rtos_pthread_mutex* mux = (struct rtos_pthread_mutex*)*mutex;

    // check if mux is busy
    int res = pthread_mutex_lock_internal(mux, 0);
    if (res == EBUSY) {
        return EBUSY;
    }

    sem_delete(mux->sem);
    kfree(mux);

    return 0;
}

static int pthread_mutex_init_if_static(pthread_mutex_t *mutex) {
    int res = 0;
    if (*mutex == PTHREAD_MUTEX_INITIALIZER) {
        portENTER_CRITICAL();
        if (*mutex == PTHREAD_MUTEX_INITIALIZER) {
            res = pthread_mutex_init(mutex, NULL);
        }
        portEXIT_CRITICAL();
    }
    return res;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    if (!mutex) {
        return EINVAL;
    }
    int res = pthread_mutex_init_if_static(mutex);
    if (res != 0) {
        return res;
    }
    return pthread_mutex_lock_internal((struct rtos_pthread_mutex *)*mutex, portMAX_DELAY);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    if (!mutex) {
        return EINVAL;
    }
    int res = pthread_mutex_init_if_static(mutex);
    if (res != 0) {
        return res;
    }
    return pthread_mutex_lock_internal((struct rtos_pthread_mutex *)*mutex, 0);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    struct rtos_pthread_mutex *mux;

    if (!mutex) {
        return EINVAL;
    }
    mux = (struct rtos_pthread_mutex *)*mutex;

    if (mux->type == PTHREAD_MUTEX_RECURSIVE) {
        mutex_recursive_give(mux->sem);
    } else {
        sem_post(mux->sem);
    }
    return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    if (!attr) {
        return EINVAL;
    }
    attr->type = PTHREAD_MUTEX_NORMAL;
    attr->is_initialized = 1;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    if (!attr) {
        return EINVAL;
    }
    attr->is_initialized = 0;
    return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
    return ENOSYS;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
    if (!attr) {
        return EINVAL;
    }
    pthread_mutexattr_t tmp_attr = {.type = type};
    int res = mutexattr_check(&tmp_attr);
    if (!res) {
        attr->type = type;
    }
    return res;
}
/****************************************/
