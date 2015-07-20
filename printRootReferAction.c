/*
 * Copyright (C) Zhang Zq (Jangzq)
 */


#include <jvmti.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "jvmtiUtil.h"
#include "jclass.h"
#include "jmethod.h"
#include "jthread.h"

#define ROOT_OUTPUT_FILE  "ujvmrootref.dat"

typedef struct RootElementData {
	jvmtiHeapReferenceKind referenceKind;
	jlong classTag;
	jlong size;
	jint length;
	jmethodID methodId;
	jlong threadTag;

	struct RootElementData * next;
} RootElement;

typedef struct {
	FILE * fp;
	RootElement * rootElements;
	RootElement * elementsTail;
	ClassModule classModule;
	MethodModule methodModule;
	ThreadModule threadModule;
} RootContext;

static void addRootElement(RootContext * context, RootElement * element) {
	if (context->elementsTail == NULL) {
		context->rootElements = element;
		context->elementsTail = element;
	} else {
		context->elementsTail->next = element;
		context->elementsTail = element;
	}
}

static void freeRootElements(RootContext * context) {
	RootElement * thisElment = NULL;
	RootElement * nextElment = NULL;

	thisElment = context->rootElements;
	while (thisElment != NULL) {
		nextElment = thisElment->next;
		free(thisElment);
		thisElment = nextElment;
	}
	context->rootElements = NULL;
	context->elementsTail = NULL;

}

static jint openOutputFile(RootContext * context) {
	jint rc = JNI_OK;

	context->fp = fopen(ROOT_OUTPUT_FILE, "wb");
	if (context->fp == NULL) {
		ulog("open output file error, %s\n", strerror(errno));
		rc = JNI_ERR;
		return rc;
	}
	return rc;
}

static void saveOutput(RootContext * context, jvmtiHeapReferenceKind reference_kind,
		const jvmtiHeapReferenceInfo* reference_info, jlong class_tag,
		jlong size, jlong* tag_ptr, jint length) {
	RootElement * element = NULL;
	ClassModule * classModule = &(context->classModule);

	if (isJClass(classModule, class_tag)) {
		return;
	}
	if (reference_kind != JVMTI_HEAP_REFERENCE_STACK_LOCAL
			&& reference_kind != JVMTI_HEAP_REFERENCE_JNI_LOCAL
			&& reference_kind != JVMTI_HEAP_REFERENCE_JNI_GLOBAL) {
		return;
	}
	element = calloc(1, sizeof(RootElement));
	if (element == NULL) {
		ulog("out of memory.");
		return;
	}
	element->referenceKind = reference_kind;
	element->classTag = getActualClassTag(classModule, class_tag, tag_ptr);
	element->size = size;
	element->length = length;
	switch (reference_kind) {
	case JVMTI_HEAP_REFERENCE_STACK_LOCAL:
		element->threadTag = reference_info->stack_local.thread_tag;
		element->methodId = reference_info->stack_local.method;
		break;
	case JVMTI_HEAP_REFERENCE_JNI_LOCAL:
		element->threadTag = reference_info->jni_local.thread_tag;
		element->methodId = reference_info->jni_local.method;
		break;
	default:
		break;
	}
	addRootElement(context, element);
}

static void printOutputElement(RootContext * context, jvmtiEnv *jvmti, RootElement * element) {
	ClassModule * classModule = &(context->classModule);
	classInfo * classInfo = getClassInfo(classModule, element->classTag);
	jint rc;
	MethodInfo * methodInfo;
	ThreadModule * threadModule = &(context->threadModule);
	MethodModule * methodModule = &(context->methodModule);

	if (classInfo == NULL) {
		return;
	}

	fprintf(context->fp, "Reference Kind: %s\n"
			"Referree: %s\n"
			"%s: %d\n", getReferenceKindStr(element->referenceKind),
			getClassName(classModule, element->classTag),
			element->length != -1 ? "Array Length" : "Self Size",
			element->length != -1 ? element->length : (jint) element->size);
	if (element->threadTag) {
		fprintf(context->fp, "Thread: %s\n", getThreadName(threadModule, element->threadTag));
	}
	if (element->methodId) {
		char * className;

		rc = getMethodInfo(methodModule, jvmti, element->methodId, &methodInfo);
		className = getClassName(classModule, methodInfo->classTag);
		if (rc == JNI_OK) {
			fprintf(context->fp, "Method: %s.%s %s\n", className, methodInfo->name,
					methodInfo->sig);
		}
	}
	fprintf(context->fp, "-----------------------------------------\n");
}

static void printOutputElements(RootContext * context, jvmtiEnv *jvmti) {
	RootElement * thisElement = context->rootElements;

	while (thisElement != NULL) {
		printOutputElement(context, jvmti, thisElement);
		thisElement = thisElement->next;
	}

}

static jint JNICALL printRootCb(jvmtiHeapReferenceKind reference_kind,
		const jvmtiHeapReferenceInfo* reference_info, jlong class_tag,
		jlong referrer_class_tag, jlong size, jlong* tag_ptr,
		jlong* referrer_tag_ptr, jint length, void* user_data) {
	RootContext * context = (RootContext *) user_data;

	switch (reference_kind) {
	case JVMTI_HEAP_REFERENCE_JNI_LOCAL:
	case JVMTI_HEAP_REFERENCE_STACK_LOCAL:
	case JVMTI_HEAP_REFERENCE_JNI_GLOBAL:
		saveOutput(context, reference_kind, reference_info, class_tag, size, tag_ptr,
				length);
		break;
	default:
		break;
	}
	return JVMTI_VISIT_OBJECTS;
}

jint printRootReferAction(jvmtiEnv *jvmti, char * options) {
	jint rc;
	jint result = JNI_ERR;
	jvmtiError err;
	jvmtiHeapCallbacks heapCallbacks;
	jvmtiCapabilities capabilities;
	RootContext context;

	ClassModule * classModule = &(context.classModule);
	ThreadModule * threadModule = &(context.threadModule);
	MethodModule * methodModule = &(context.methodModule);

	jlong currentTag;

	memset(&context, 0, sizeof(RootContext));

	memset(&capabilities, 0, sizeof(capabilities));
	capabilities.can_tag_objects = 1;
	err = (*jvmti)->AddCapabilities(jvmti, &capabilities);
	CHECK_ERROR_AND_RETURN(jvmti, err, "add capabilities", JNI_ERR);

	rc = initClassModule(classModule, jvmti, 1);
	if (rc == JNI_ERR) {
		ulog("Load class module error\n");
		goto error;
	}
#ifdef DEBUG
	ulog("init class module finish.\n");
#endif

	currentTag = getClassCount(classModule) + 100;

	rc = initThreadModule(threadModule, jvmti, currentTag);
	if (rc == JNI_ERR) {
		ulog("Load thread error\n");
		goto error;
	}

#ifdef DEBUG
	ulog("init thread module finish.\n");
#endif

	rc = initMethodModule(methodModule);
	if (rc == JNI_ERR) {
		ulog("init method error\n");
		goto error;
	}

#ifdef DEBUG
	ulog("init method module finish.\n");
#endif

	memset(&heapCallbacks, 0, sizeof(heapCallbacks));
	heapCallbacks.heap_reference_callback = &printRootCb;

	rc = openOutputFile(&context);
	if (rc == JNI_ERR) {
		goto error;
	}

	err = (*jvmti)->FollowReferences(jvmti, 0, NULL, NULL, &heapCallbacks,
	&context);
	print_if_error(jvmti, err, "FollowReferences error");
	if (err != JVMTI_ERROR_NONE) {
		goto error;
	}
	printOutputElements(&context, jvmti);

	result = JNI_OK;

	error: if (context.fp != NULL) {
		fclose(context.fp);
	}

	closeClassModule(classModule);
#ifdef DEBUG
	ulog("close class module finish.\n");
#endif

	closeThreadModule(threadModule);
#ifdef DEBUG
	ulog("close thread module finish.\n");
#endif

	closeMethodModule(methodModule);
#ifdef DEBUG
	ulog("close method module finish.\n");
#endif

	if (context.rootElements != NULL) {
		freeRootElements(&context);
	}
	err = (*jvmti)->RelinquishCapabilities(jvmti, &capabilities);
	CHECK_ERROR_AND_RETURN(jvmti, err, "Relinquish Capablilities error",
			JNI_ERR);

	return result;
}

