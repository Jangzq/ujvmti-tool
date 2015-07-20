/*
 * Copyright (C) Zhang Zq (Jangzq)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "jmethod.h"
#include "jvmtiUtil.h"


/**
 * get method name by methodId.
 */
static jint getMethodName(jvmtiEnv *jvmti, jmethodID methodId,
		char ** methodName, char ** sig, jlong * classTag) {
	char * _methodName;
	char * _sig;
	jclass declaringClass;
	jvmtiError err;

	err = (*jvmti)->GetMethodDeclaringClass(jvmti, methodId, &declaringClass);
	CHECK_ERROR_AND_RETURN(jvmti, err, "GetMethodDeclaringClass error", JNI_ERR);

	err = (*jvmti)->GetTag(jvmti, declaringClass, classTag);
	CHECK_ERROR_AND_RETURN(jvmti, err, "GetTag error", JNI_ERR);


	err = (*jvmti)->GetMethodName(jvmti, methodId, &_methodName, &_sig, NULL);
	CHECK_ERROR_AND_RETURN(jvmti, err, "get method name error", JNI_ERR);

	*methodName = strdup(_methodName);
	*sig = strdup(_sig);

	deallocate(jvmti, _methodName);
	deallocate(jvmti, _sig);

	return JNI_OK;
}

/*******************Public Functions*************************/
jint initMethodModule(MethodModule * methodModule) {
	methodModule->currentMethodPos = 0;
	methodModule->currentMethodSize = INIT_METHOD_SIZE;
	methodModule->methodInfos = calloc(methodModule->currentMethodSize, sizeof(MethodInfo));
	return JNI_OK;
}

jint closeMethodModule(MethodModule * methodModule) {
	MethodInfo * methodInfos = methodModule->methodInfos;
	if (methodInfos != NULL) {
		int i;

		for (i = 0; i < methodModule->currentMethodPos; i++) {
			if (methodInfos[i].name != NULL) {
				free(methodInfos[i].name);
			}
			if (methodInfos[i].sig != NULL) {
				free(methodInfos[i].sig);
			}
		}
		free(methodInfos);
	}
	return JNI_OK;
}

jint getMethodInfo( MethodModule * methodModule, jvmtiEnv *jvmti, jmethodID method, MethodInfo ** methodInfo) {
	int i;
	jint rc;
	MethodInfo * methodInfos = methodModule->methodInfos;

	for (i = 0; i < methodModule->currentMethodPos; i++) {
		if (methodInfos[i].methodId == method) {
			*methodInfo = methodInfos + i;
			return JNI_OK;
		}
	}

	if (methodModule->currentMethodPos >= methodModule->currentMethodSize) {
		int newSize = 2 * methodModule->currentMethodSize;
		MethodInfo *newMethods = realloc(methodInfos,
				newSize * sizeof(MethodInfo));

		if (newMethods == NULL) {
			ulog("realloc error\n");
			return JNI_ERR;
		}
		methodModule->methodInfos = methodInfos = newMethods;
		methodModule->currentMethodSize = newSize;
	}
	methodInfos[methodModule->currentMethodPos].methodId = method;
	rc = getMethodName(jvmti, method, &(methodInfos[methodModule->currentMethodPos].name),
			&(methodInfos[methodModule->currentMethodPos].sig), &(methodInfos[methodModule->currentMethodPos].classTag));
	if(rc == JNI_ERR) {
		return JNI_ERR;
	}
	*methodInfo = methodInfos + methodModule->currentMethodPos;
	methodModule->currentMethodPos++;
	return JNI_OK;
}

