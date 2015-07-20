/*
 * Copyright (C) Zhang Zq (Jangzq)
 */

#ifndef JVMTIUTIL_H_
#define JVMTIUTIL_H_


#include <stdio.h>
#include <stdarg.h>
#include <jvmti.h>
#include <stdint.h>

typedef uint32_t uint32;
typedef uint8_t byte;
typedef uint16_t ushort;
typedef byte bool;

#define false 0
#define true 1


extern void ulog(const char * format, ...);
extern void print_if_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str);
extern void deallocate(jvmtiEnv *jvmti, void *ptr);
extern char * getReferenceKindStr(jvmtiHeapReferenceKind reference_kind);

#define CHECK_ERROR_AND_RETURN(jvmti, err, str, errorReturn) \
	do {                 \
		print_if_error(jvmti, err, str);  \
		if(err != JVMTI_ERROR_NONE) {   \
			return errorReturn;  \
		}  \
	} while(0);

#endif /* JVMTIUTIL_H_ */
