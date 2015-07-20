/*
 * Copyright (C) Zhang Zq (Jangzq)
 */


#include <jvmti.h>

#ifndef JMETHOD_H_
#define JMETHOD_H_

#ifdef DEBUG
#define INIT_METHOD_SIZE 1
#else
#define INIT_METHOD_SIZE 10
#endif


typedef struct {
	char * name;
	char * sig;
	jlong classTag;
	jmethodID methodId;
} MethodInfo;


typedef struct {
	MethodInfo * methodInfos;
	int currentMethodPos;
	int currentMethodSize;
} MethodModule;

extern jint initMethodModule(MethodModule * methodModule);
extern jint closeMethodModule(MethodModule * methodModule);
extern jint getMethodInfo(MethodModule * methodModule, jvmtiEnv *jvmti, jmethodID method, MethodInfo ** methodInfo);


#endif /* JMETHOD_H_ */
