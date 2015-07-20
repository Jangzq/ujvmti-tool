/*
 * Copyright (C) Zhang Zq (Jangzq)
 */


#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "jvmtiUtil.h"
#include "jthread.h"



/*******************************Private Functions***********************************/

static void freeThreadInfo(ThreadModule * threadModule) {
	int i;

	for (i = 0; i < threadModule->threadCount; i++) {
		if (threadModule->threadInfos[i].name != NULL) {
			free(threadModule->threadInfos[i].name);
		}
	}

	free(threadModule->threadInfos);
	threadModule->threadInfos = NULL;
}

static jint tagThreads(ThreadModule * threadModule, jvmtiEnv * jvmti, jlong beginTag) {
	jthread * threads;
	jvmtiError err;
	int i;

	err = (*jvmti)->GetAllThreads(jvmti, &threadModule->threadCount, &threads);
	CHECK_ERROR_AND_RETURN(jvmti, err, "get all thread error", JNI_ERR);

	threadModule->threadInfos = calloc(threadModule->threadCount, sizeof(ThreadInfo));
	for (i = 0; i < threadModule->threadCount; i++) {
		jvmtiThreadInfo aInfo;

		int tag = beginTag + i;

		err = (*jvmti)->GetThreadInfo(jvmti, threads[i], &aInfo);
		print_if_error(jvmti, err, "get thread info error");
		if (err == JVMTI_ERROR_NONE) {
			(*jvmti)->SetTag(jvmti, threads[i], tag);
			threadModule->threadInfos[i].name = strdup(aInfo.name);
#ifdef DEBUG
			ulog("tag thread: %s to %d\n", threadModule->threadInfos[i].name, tag);
#endif
			deallocate(jvmti, aInfo.name);
		}
	}

	deallocate(jvmti, threads);
	return JNI_OK;
}

/*******************************Public Functions***********************************/
jint initThreadModule(ThreadModule * threadModule,  jvmtiEnv * jvmti, jlong _beginTag) {
	jint rc;

	threadModule->beginTag = _beginTag;
	rc = tagThreads(threadModule, jvmti, threadModule->beginTag);

	return rc;
}

jint closeThreadModule(ThreadModule * threadModule) {
	if (threadModule->threadInfos != NULL) {
		freeThreadInfo(threadModule);
		threadModule->threadInfos = NULL;
	}

	return JNI_OK;
}

/**
 * Get thread name by tag.
 */
char * getThreadName(ThreadModule * threadModule, jlong tag) {
	tag -= threadModule->beginTag;
	if (tag >= threadModule->threadCount || tag < 0) {
		return NULL;
	}
	return threadModule->threadInfos[tag].name;
}

