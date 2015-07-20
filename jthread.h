/*
 * Copyright (C) Zhang Zq (Jangzq)
 */



#ifndef JTHREAD_H_
#define JTHREAD_H_

typedef struct {
	char * name;
} ThreadInfo;

typedef struct {
	ThreadInfo * threadInfos;
	int threadCount;
	int beginTag;
} ThreadModule;

extern jint initThreadModule(ThreadModule * threadModule, jvmtiEnv * jvmti, jlong _beginTag);
extern jint closeThreadModule(ThreadModule *threadModule);
extern char * getThreadName(ThreadModule * threadModule, jlong tag);


#endif /* JTHREAD_H_ */
