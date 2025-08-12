/**
************************************************************************************************************
*               Copyright(c) 2014-2015, Realtek Semiconductor Corporation. All rights reserved.
************************************************************************************************************
* @file     compiler_abstraction.h
* @brief    Compiler specific intrinsics
* @author   lory_xu
* @date     2015-03
* @version  v0.1
*************************************************************************************************************
*/

#ifndef __COMPILER_ABSTRACTION_H__
#define __COMPILER_ABSTRACTION_H__

/** @brief Define compilor specific symbol */

#if defined ( __CC_ARM )
#define __STATIC_INLINE         static __inline
#define __STATIC_ALWAYS_INLINE  static __forceinline
#define barrier()               __memory_changed()
#define __UNUSED                __attribute__ ((unused))

#ifndef portFORCE_INLINE
#define portFORCE_INLINE    __forceinline
#endif

#elif defined ( __ICCARM__ )
#define __STATIC_INLINE         static inlinef

#ifndef portFORCE_INLINE
#define portFORCE_INLINE    __attribute__((always_inline))
#endif

#elif defined ( __GNUC__ )
#define __STATIC_INLINE         static inline
#define __STATIC_ALWAYS_INLINE  static inline __attribute__ ((always_inline))
#define barrier()               __asm volatile ("": : :"memory")
#define __UNUSED                __attribute__ ((unused))

#ifndef portFORCE_INLINE
#define portFORCE_INLINE    __attribute__((always_inline)) inline
#endif

#ifndef __forceinline
#define __forceinline       __attribute__((always_inline)) inline
#endif

#ifndef __weak
#define  __weak             __attribute__( ( weak ) )
#endif
#endif

#endif /* __COMPILER_ABSTRACTION_H__ */
