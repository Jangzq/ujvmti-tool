/*
 * Copyright (C) Zhang Zq (Jangzq)
 */


#ifndef JCLASS_H_
#define JCLASS_H_

#include <stdlib.h>
#include <string.h>

#include "jvmtiUtil.h"

typedef struct {
	char * classSig;
	bool follow;
	char * fieldName;
	char * fieldSig;
} classInfo;

typedef struct {
	classInfo * classes;
	int classCount;
	int jclassTag;
	int beginTag;
} ClassModule;


extern jint initClassModule(ClassModule * classModule, jvmtiEnv * jvmti, jlong _beginTag);
extern jint closeClassModule(ClassModule * classModule);
extern char * getClassName(ClassModule * classModule, jlong tag);
extern bool isJClass(ClassModule * classModule, jlong tag);
extern bool isFollow(ClassModule * classModule, jlong tag);
extern jint followClasses(ClassModule * classModule, const char * fileName);
extern classInfo * getClassInfo(ClassModule * classModule, jlong tag);

extern jint getObjectFieldValue(ClassModule * classModule, jvmtiEnv *jvmti, JNIEnv *jniEnv, jlong objTag,
		char * fieldName, char * fieldSig, int * retValue);
extern int getClassCount(ClassModule * classModule);

extern jlong getActualClassTag(ClassModule * classModule, jlong classTag, jlong* tag_ptr);

#endif /* JCLASS_H_ */
