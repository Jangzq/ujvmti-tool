/*
 * Copyright (C) Zhang Zq (Jangzq)
 */


#include <jvmti.h>

#include "jvmtiUtil.h"
#include <string.h>
#include <stdlib.h>

#define DUMP_REFER   "dump_refer"
#define DUMP_ROOT   "dump_root"

extern jint printReferAction(jvmtiEnv *jvmti, JNIEnv *jniEnv, char * otherOption);
extern jint printRootReferAction(jvmtiEnv *jvmti, char * options);

/**
 * the return value need not be freed.
 */
static jint parseOption(char * options, char ** optionType, char ** otherOption) {
	int optionTypeLen;
	int optionsLen = strlen(options);

	if (options == NULL) {
		return JNI_ERR;
	}

	*optionType = strtok(options, ";");
	optionTypeLen = strlen(*optionType);
	if (optionTypeLen == optionsLen) {
		*otherOption = NULL;
	} else {
		*otherOption = options + strlen(*optionType) + 1;
	}
	return JNI_OK;
}

JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM *vm, char *options, void *reserved) {
	jint rc;
	jvmtiEnv *jvmti;
	JNIEnv *jniEnv;
	jvmtiError err;


	char * optionType;
	char * otherOption = NULL;

	/* Get JVMTI environment */
	jvmti = NULL;
	rc = (*vm)->GetEnv(vm, (void **) &jvmti, JVMTI_VERSION);
	if (rc != JNI_OK) {
		ulog("ERROR: Unable to create jvmtiEnv, error=%d\n", rc);
		return JNI_ERR;
	}
	if (jvmti == NULL) {
		ulog("ERROR: No jvmtiEnv* returned from GetEnv\n");
		return JNI_ERR;
	}
	/////////////jni/////////////
	rc = (*vm)->GetEnv(vm, (void **) &jniEnv, JNI_VERSION_1_2);
	if (rc != JNI_OK) {
		ulog("ERROR: Unable to create jnienv, error=%d\n", rc);
		return JNI_ERR;
	}
	if (jvmti == NULL) {
		ulog("ERROR: No jnienv* returned from GetEnv\n");
		return JNI_ERR;
	}
	////////////////////////////
#ifdef DEBUG
	ulog("options: %s\n", options);
#endif

	parseOption(options, &optionType, &otherOption);

#ifdef DEBUG
	ulog("optionType:%s, otherOption: %s\n", optionType, otherOption==NULL?"":otherOption);
#endif


	if (strcmp(optionType, DUMP_REFER) == 0) {
		rc = printReferAction(jvmti, jniEnv, otherOption);
	} else if (strcmp(optionType, DUMP_ROOT) == 0) {
		rc = printRootReferAction(jvmti, otherOption);
	} else {
		rc = JNI_ERR;
		ulog("ERROR: invalid options\n");
	}

	err = (*jvmti)->DisposeEnvironment(jvmti);
	CHECK_ERROR_AND_RETURN(jvmti, err, "DisposeEnvironment error", JNI_ERR);

	return rc;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) {
}

