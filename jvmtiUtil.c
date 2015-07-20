/*
 * Copyright (C) Zhang Zq (Jangzq)
 */


#include <stdlib.h>
#include <string.h>

#include "jvmtiUtil.h"

void ulog(const char * format, ...) {
	va_list ap;

	va_start(ap, format);
	(void) vfprintf(stdout, format, ap);
	fflush(stdout);
	va_end(ap);
}

void print_if_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
	if (errnum != JVMTI_ERROR_NONE) {
		char *errnum_str;

		errnum_str = NULL;
		(void) (*jvmti)->GetErrorName(jvmti, errnum, &errnum_str);

		ulog("ERROR: JVMTI: %d(%s): %s\n", errnum,
				(errnum_str == NULL ? "Unknown" : errnum_str),
				(str == NULL ? "" : str));
	}
}

void deallocate(jvmtiEnv *jvmti, void *ptr) {
	jvmtiError error;

	error = (*jvmti)->Deallocate(jvmti, ptr);
	print_if_error(jvmti, error, "Cannot deallocate memory");
}

char * getReferenceKindStr(jvmtiHeapReferenceKind reference_kind) {
	switch (reference_kind) {
	case JVMTI_HEAP_REFERENCE_FIELD:
		return "JVMTI_HEAP_REFERENCE_FIELD";
	case JVMTI_HEAP_REFERENCE_ARRAY_ELEMENT:
		return "JVMTI_HEAP_REFERENCE_ARRAY_ELEMENT";
	case JVMTI_HEAP_REFERENCE_STATIC_FIELD:
		return "JVMTI_HEAP_REFERENCE_STATIC_FIELD";
	case JVMTI_HEAP_REFERENCE_JNI_GLOBAL:
		return "JVMTI_HEAP_REFERENCE_JNI_GLOBAL";
	case JVMTI_HEAP_REFERENCE_JNI_LOCAL:
		return "JVMTI_HEAP_REFERENCE_JNI_LOCAL";
	case JVMTI_HEAP_REFERENCE_STACK_LOCAL:
		return "JVMTI_HEAP_REFERENCE_STACK_LOCAL";
	default:
		return "OTHER";
	}

}


