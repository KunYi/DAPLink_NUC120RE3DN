#ifndef TUSB_OSAL_THREADX_H_
#define TUSB_OSAL_THREADX_H_

// ThreadX header
#include "tx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// TASK API
//--------------------------------------------------------------------+
#define MS_TO_TICKS(ms) (((ms) * TX_TIMER_TICKS_PER_SECOND + 999) / 1000)

TU_ATTR_ALWAYS_INLINE static inline uint32_t _osal_ms2tick(uint32_t msec) {
  if ( msec == OSAL_TIMEOUT_WAIT_FOREVER ) return TX_WAIT_FOREVER;
  if ( msec == 0 ) return 0;

  uint32_t ticks = MS_TO_TICKS(msec);

  if ( ticks == 0 ) ticks = 1;

  return ticks;
}

TU_ATTR_ALWAYS_INLINE static inline void osal_task_delay(uint32_t msec) {
  tx_thread_sleep(_osal_ms2tick(msec));
}
//--------------------------------------------------------------------+
// Semaphore API
//--------------------------------------------------------------------+
typedef struct TX_SEMAPHORE_STRUCT osal_semaphore_def_t;
typedef struct TX_SEMAPHORE_STRUCT* osal_semaphore_t;

TU_ATTR_ALWAYS_INLINE static inline
osal_semaphore_t osal_semaphore_create(osal_semaphore_def_t *semdef) {
  return (TX_SUCCESS == tx_semaphore_create(semdef, "tusb", 1)) ? semdef : NULL;
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_semaphore_delete(osal_semaphore_t semd_hdl) {
  return (TX_SUCCESS == tx_semaphore_delete(semd_hdl));
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_semaphore_post(osal_semaphore_t sem_hdl, bool in_isr) {
  (void) in_isr;
  return (TX_SUCCESS == tx_semaphore_put(sem_hdl));
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_semaphore_wait(osal_semaphore_t sem_hdl, uint32_t msec) {
  return (TX_SUCCESS == tx_semaphore_get(sem_hdl, _osal_ms2tick(msec)));
}

TU_ATTR_ALWAYS_INLINE static inline void osal_semaphore_reset(osal_semaphore_t const sem_hdl) {
    // TODO: implement

}

//--------------------------------------------------------------------+
// MUTEX API (priority inheritance)
//--------------------------------------------------------------------+
typedef struct TX_MUTEX_STRUCT  osal_mutex_def_t;
typedef struct TX_MUTEX_STRUCT* osal_mutex_t;

TU_ATTR_ALWAYS_INLINE static inline osal_mutex_t osal_mutex_create(osal_mutex_def_t* mdef) {
  return (TX_SUCCESS == tx_mutex_create(mdef, "tusb", TX_INHERIT)) ? mdef : NULL;
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_mutex_delete(osal_mutex_t mutex_hdl) {
  return (TX_SUCCESS == tx_mutex_delete(mutex_hdl));
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_mutex_lock(osal_mutex_t mutex_hdl, uint32_t msec) {
  return (TX_SUCCESS == tx_mutex_get(mutex_hdl, _osal_ms2tick(msec)));
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_mutex_unlock(osal_mutex_t mutex_hdl) {
  return (TX_SUCCESS == tx_mutex_put(mutex_hdl));
}

//--------------------------------------------------------------------+
// QUEUE API
//--------------------------------------------------------------------+
typedef struct
{
  uint16_t depth;
  uint16_t item_sz;
  void*    buf;
  char 	   *name;
  struct TX_QUEUE_STRUCT tq;
} osal_queue_def_t;
typedef struct TX_QUEUE_STRUCT* osal_queue_t;

#define _OSAL_Q_NAME(_name) .name = #_name
#define ITEM_SIZE_UL(_type)  ((sizeof(_type) + sizeof(ULONG) - 1) / sizeof(ULONG))

#define OSAL_QUEUE_DEF(_int_set, _name, _depth, _type) \
  static _type _name##_##buf[_depth];\
  osal_queue_def_t _name = \
  	{ .depth = _depth, \
	  .item_sz = ITEM_SIZE_UL(_type), \
	  .buf = _name##_##buf, \
	  _OSAL_Q_NAME(_name) \
        }

TU_ATTR_ALWAYS_INLINE static inline osal_queue_t osal_queue_create(osal_queue_def_t* qdef) {

  return (TX_SUCCESS == tx_queue_create(&qdef->tq,
		qdef->name,
		qdef->item_sz,
		qdef->buf,
		qdef->depth * qdef->item_sz)) ?
		&qdef->tq : NULL;
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_queue_delete(osal_queue_t qhdl) {
  return (TX_SUCCESS == tx_queue_delete(qhdl));
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_queue_receive(osal_queue_t qhdl, void* data, uint32_t msec) {
  return (TX_SUCCESS == tx_queue_receive(qhdl, data, _osal_ms2tick(msec)));
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_queue_send(osal_queue_t qhdl, void const* data, bool in_isr) {
  return (TX_SUCCESS == tx_queue_send(qhdl, (void *)data,  in_isr ? TX_NO_WAIT : TX_WAIT_FOREVER));
}

TU_ATTR_ALWAYS_INLINE static inline bool osal_queue_empty(osal_queue_t qhdl) {
  return (TX_SUCCESS == tx_queue_flush(qhdl));
}

#ifdef __cplusplus
}
#endif

#endif
