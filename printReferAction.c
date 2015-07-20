/*
 * Copyright (C) Zhang Zq (Jangzq)
 */


#include <jvmti.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "jvmtiUtil.h"
#include "jclass.h"

#define OUTPUT_FILE  "ujvmheapref.dat"
#define DEFAULT_FILTER_FILE "ujvmfilter.cfg"

typedef struct OutputElementData {
	jvmtiHeapReferenceKind referenceKind;
	jlong referrerClassTag;
	jlong classTag;
	jlong objTag;
	jlong size;
	jint length;
	struct OutputElementData * next;
} OutputElement;

typedef struct {
	FILE * fp;
	jlong currentTag;
	OutputElement * outputElements;
	OutputElement * elementsTail;
	ClassModule classModule;
} ReferenceContext;


static void addOutputElement(ReferenceContext * context, OutputElement * element) {
	if (context->elementsTail == NULL) {
		context->outputElements = element;
		context->elementsTail = element;
	} else {
		context->elementsTail->next = element;
		context->elementsTail = element;
	}
}

static void freeOutputElements(ReferenceContext * context) {
	OutputElement * thisElment = NULL;
	OutputElement * nextElment = NULL;

	thisElment = context->outputElements;
	while (thisElment != NULL) {
		nextElment = thisElment->next;
		free(thisElment);
		thisElment = nextElment;
	}
	context->outputElements = NULL;
	context->elementsTail = NULL;

}

static jlong setObjTag(ReferenceContext * context, jlong * ptr) {
	if (*ptr == 0) {
		*ptr = context->currentTag++;
	}
	return  *ptr;
}

static jint openOutputFile(ReferenceContext * context) {
	jint rc = JNI_OK;

	context->fp = fopen(OUTPUT_FILE, "wb");
	if (context->fp == NULL) {
		ulog("open output file error, %s\n", strerror(errno));
		rc = JNI_ERR;
		return rc;
	}
	return rc;
}

static void saveOutput(ReferenceContext * context, jvmtiHeapReferenceKind reference_kind, jlong class_tag,
		jlong referrer_class_tag, jlong size, jlong* tag_ptr,
		jlong* referrer_tag_ptr, jint length) {
	ClassModule * classModule = &(context->classModule);

	if (isJClass(classModule, class_tag)) {
		return;
	}
	classInfo * classInfo = getClassInfo(classModule, class_tag);
	if (classInfo == NULL) {
		return;
	}
	if (!classInfo->follow) {
		return;
	}
	jlong objTag;

	objTag = setObjTag(context, tag_ptr);
	OutputElement * element = calloc(1, sizeof(OutputElement));
	if (element == NULL) {
		ulog("out of memory.");
		return;
	}
	element->referenceKind = reference_kind;
	element->referrerClassTag = getActualClassTag(classModule, referrer_class_tag,
			referrer_tag_ptr);
	element->classTag = class_tag;
	element->objTag = objTag;
	element->size = size;
	element->length = length;

	addOutputElement(context, element);

}

static void printOutputElement(ReferenceContext * context, jvmtiEnv *jvmti, JNIEnv *jniEnv,
		OutputElement * element) {
	ClassModule * classModule = &(context->classModule);
	classInfo * classInfo = getClassInfo(classModule, element->classTag);
	jint rc;

	if (classInfo == NULL) {
		return;
	}

	fprintf(context->fp, "Reference Kind: %s\n"
			"Referrer: %s  Referree: %s\n"
			"%s: %d\n", getReferenceKindStr(element->referenceKind),
			getClassName(classModule, element->referrerClassTag),
			getClassName(classModule, element->classTag),
			element->length != -1 ? "Array Length" : "Self Size",
			element->length != -1 ? element->length : (jint)element->size);

	if (classInfo->fieldName != NULL) {
		int value;

		rc = getObjectFieldValue(classModule, jvmti, jniEnv, element->objTag,
				classInfo->fieldName, classInfo->fieldSig, &value);
		if (rc == JNI_ERR) {
			return;
		}
		fprintf(context->fp, "%s: %d\n",
				classInfo->fieldName, value);
	}
	fprintf(context->fp, "-----------------------------------------\n");
}

static void printOutputElements(ReferenceContext * context, jvmtiEnv *jvmti, JNIEnv *jniEnv) {
	OutputElement * thisElement = context->outputElements;

	while (thisElement != NULL) {
		printOutputElement(context, jvmti, jniEnv, thisElement);
		thisElement = thisElement->next;
	}

}



static jint JNICALL printReferCb(jvmtiHeapReferenceKind reference_kind,
		const jvmtiHeapReferenceInfo* reference_info, jlong class_tag,
		jlong referrer_class_tag, jlong size, jlong* tag_ptr,
		jlong* referrer_tag_ptr, jint length, void* user_data) {
	ReferenceContext * context = (ReferenceContext *) user_data;

	switch (reference_kind) {
	case JVMTI_HEAP_REFERENCE_FIELD:
	case JVMTI_HEAP_REFERENCE_ARRAY_ELEMENT:
	case JVMTI_HEAP_REFERENCE_STATIC_FIELD:
	case JVMTI_HEAP_REFERENCE_JNI_GLOBAL:
	case JVMTI_HEAP_REFERENCE_JNI_LOCAL:
	case JVMTI_HEAP_REFERENCE_STACK_LOCAL:
		saveOutput(context, reference_kind, class_tag, referrer_class_tag, size, tag_ptr,
				referrer_tag_ptr, length);
		break;
	default:
		break;
	}
	return JVMTI_VISIT_OBJECTS;
}
jint printReferAction(jvmtiEnv *jvmti, JNIEnv *jniEnv, char * options) {
	jint rc;
	jint result = JNI_ERR;
	jvmtiError err;
	jvmtiHeapCallbacks heapCallbacks;
	jvmtiCapabilities capabilities;
	char * filterFileName = NULL;
	ReferenceContext context;
	ClassModule * classModule = &(context.classModule);

	memset(&context, 0, sizeof(ReferenceContext));

	memset(&capabilities, 0, sizeof(capabilities));
	capabilities.can_tag_objects = 1;
	err = (*jvmti)->AddCapabilities(jvmti, &capabilities);
	CHECK_ERROR_AND_RETURN(jvmti, err, "add capabilities", JNI_ERR);

	if (options == NULL) {
		ulog("Filter file isn't specified, use defaut.\n");
		filterFileName = DEFAULT_FILTER_FILE;
	} else {
		filterFileName = options;
	}

	rc = initClassModule(classModule, jvmti, 1);
	if (rc == JNI_ERR) {
		ulog("Load class module error\n");
		goto error;
	}

	context.currentTag = getClassCount(classModule) + 100;

	rc = followClasses(classModule, filterFileName);
	if (rc == JNI_ERR) {
		goto error;
	}

	memset(&heapCallbacks, 0, sizeof(heapCallbacks));
	heapCallbacks.heap_reference_callback = &printReferCb;

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
	printOutputElements(&context, jvmti, jniEnv);

	result = JNI_OK;

	error: if (context.fp != NULL) {
		fclose(context.fp);
	}

	closeClassModule(classModule);

	if (context.outputElements != NULL) {
		freeOutputElements(&context);
	}
	err = (*jvmti)->RelinquishCapabilities(jvmti, &capabilities);
	CHECK_ERROR_AND_RETURN(jvmti, err, "Relinquish Capablilities error",
			JNI_ERR);

	return result;
}

