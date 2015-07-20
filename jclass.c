/*
 * Copyright (C) Zhang Zq (Jangzq)
 */

#include "jclass.h"
#include <jvmti.h>
#include <errno.h>

/*******************************Private Functions***********************************/

#define IS_SPACE(ch) (ch == ' ' || ch == '\t' || ch == '\n' || ch== '\r')

static char * trim(char * str) {
	size_t len;
	int beginIndex;
	int endIndex;

	if (str == NULL) {
		return NULL;
	}
	len = strlen(str);
	for (beginIndex = 0; beginIndex < len && IS_SPACE(str[beginIndex]);
			beginIndex++)
		;
	for (endIndex = len - 1; endIndex >= beginIndex && IS_SPACE(str[endIndex]);
			endIndex--)
		;
	str[endIndex + 1] = 0;

	return str + beginIndex;
}

/**
 * Set follow information in classes.
 */
static void followClass(ClassModule * classModule, char * filterClass) {
	int i = 0;
	char * className = NULL;
	char * fieldName = NULL;
	char * fieldSig = NULL;

	className = strtok(filterClass, ",");
	fieldName = strtok(NULL, ",");
	if (fieldName != NULL) {
		fieldSig = strtok(NULL, ",");
	}
#ifdef DEBUG
	ulog("class: %s, fieldName: %s, fieldSig: %s\n", className, fieldName,
			fieldSig);
#endif

	for (i = 0; i < classModule->classCount; i++) {
		if (strcmp(classModule->classes[i].classSig, className) == 0) {
			classModule->classes[i].follow = true;
			if (fieldName != NULL && fieldSig != NULL) {
				classModule->classes[i].fieldName = strdup(fieldName);
				classModule->classes[i].fieldSig = strdup(fieldSig);
			}
#ifdef DEBUG
			ulog("Set filter class: %s\n", className);
#endif
		}
	}
}

/**
 * invoked when close class module, free the memory used by classinfo.
 */
static void freeClassInfo(ClassModule * classModule) {
	int i;

	for (i = 0; i < classModule->classCount; i++) {
		if (classModule->classes[i].classSig != NULL) {
			free(classModule->classes[i].classSig);
		}
		if (classModule->classes[i].fieldName != NULL) {
			free(classModule->classes[i].fieldName);
		}
		if (classModule->classes[i].fieldSig != NULL) {
			free(classModule->classes[i].fieldSig);
		}
	}

	free(classModule->classes);
}

/**
 * Get all classes from VM, and set the tag.
 */
static jint tagClasses(ClassModule * classModule, jvmtiEnv * jvmti,
		jlong beginTag) {
	jvmtiError err;
	jclass *jclasses;
	int jclassCount;
	classInfo *classInfos;
	int i;
	jint rc = JNI_OK;

	err = (*jvmti)->GetLoadedClasses(jvmti, &jclassCount, &jclasses);
	CHECK_ERROR_AND_RETURN(jvmti, err, "get loaded classes", JNI_ERR);

	classInfos = calloc(sizeof(classInfo), jclassCount);
	memset(classInfos, 0, sizeof(classInfo) * jclassCount);
	if (classInfos != NULL) {
		for (i = 0; i < jclassCount; i++) {
			char * sig = NULL;
			int tag = i + beginTag;
			err = (*jvmti)->GetClassSignature(jvmti, jclasses[i], &sig, NULL);
			print_if_error(jvmti, err, "get class signature");
			if (err == JVMTI_ERROR_NONE && sig != NULL) {
				if (strcmp(sig, "Ljava/lang/Class;") == 0) {
					classModule->jclassTag = tag;
				}
				classInfos[i].classSig = strdup(sig);
				deallocate(jvmti, sig);
				err = (*jvmti)->SetTag(jvmti, jclasses[i], tag);
				print_if_error(jvmti, err, "set object tag");
			}
		}
		classModule->classes = classInfos;
		classModule->classCount = jclassCount;
	} else {
		ulog("out of memory\n");
		rc = JNI_ERR;
	}
	deallocate(jvmti, jclasses);
	return rc;
}

/*******************************Public Functions***********************************/

/**
 * initialize the class module, tag all the class.
 */
jint initClassModule(ClassModule * classModule, jvmtiEnv * jvmti,
		jlong _beginTag) {
	jint rc;

	classModule->beginTag = _beginTag;
	rc = tagClasses(classModule, jvmti, classModule->beginTag);

	return rc;
}

jint closeClassModule(ClassModule * classModule) {
	if (classModule->classes != NULL) {
		freeClassInfo(classModule);
		classModule->classes = NULL;
	}
	return JNI_OK;
}

/**
 * Get class name by tag.
 */
char * getClassName(ClassModule * classModule, jlong tag) {
	tag -= classModule->beginTag;
	if (tag >= classModule->classCount || tag < 0) {
		return NULL;
	}
	return classModule->classes[tag].classSig;
}

/**
 * is the class iden by tag is 'java.lang.Class'?
 */
bool isJClass(ClassModule * classModule, jlong tag) {
	return tag == classModule->jclassTag;
}

/**
 * is the class iden by tag is follow?
 */
bool isFollow(ClassModule * classModule, jlong tag) {
	tag -= classModule->beginTag;
	if (tag >= classModule->classCount || tag < 0) {
		return false;
	}
	return classModule->classes[tag].follow;
}

/**
 * Get class info by tag.
 */
classInfo * getClassInfo(ClassModule * classModule, jlong tag) {
	tag -= classModule->beginTag;
	if (tag >= classModule->classCount || tag < 0) {
		return NULL;
	}
	return classModule->classes + (int) tag;
}

jint followClasses(ClassModule * classModule, const char * fileName) {
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	FILE * fp;
	jint rc;

	if (fileName == NULL) {
		return JNI_ERR;
	}

	fp = fopen(fileName, "r");
	if (fp == NULL) {
		ulog("open class name file (%s) error, %s\n", fileName,
				strerror(errno));
		rc = JNI_ERR;
		return rc;
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		char * newChar;

		newChar = trim(line);
		if (strlen(newChar) != 0 && newChar[0] != '#') {
#ifdef DEBUG
			ulog("Filter class: %s\n", newChar);
#endif
			followClass(classModule, newChar);
		}
	}
	free(line);
	fclose(fp);
	return JNI_OK;
}

/**
 * Get the object's field value.
 */
jint getObjectFieldValue(ClassModule * classModule, jvmtiEnv *jvmti,
		JNIEnv *jniEnv, jlong objTag, char * fieldName, char * fieldSig,
		int * retValue) {
	jint objCount;
	jobject * objs = NULL;
	jvmtiError err;
	jclass clazz;
	jfieldID id_number;
	jint fieldValue;
	jint result = JNI_ERR;

	err = (*jvmti)->GetObjectsWithTags(jvmti, 1, &objTag, &objCount, &objs,
	NULL);
	CHECK_ERROR_AND_RETURN(jvmti, err, "get object by tag error", JNI_ERR);

	if (objCount != 1) {
		goto error;
	}

	clazz = (*jniEnv)->GetObjectClass(jniEnv, *objs);
	id_number = (*jniEnv)->GetFieldID(jniEnv, clazz, fieldName, fieldSig);
	if (id_number == NULL) {
		goto error;
	}
	fieldValue = (*jniEnv)->GetIntField(jniEnv, *objs, id_number);
	*retValue = fieldValue;
	result = JNI_OK;

	error: if (objs != NULL) {
		deallocate(jvmti, objs);
	}

	return result;
}

int getClassCount(ClassModule * classModule) {
	return classModule->classCount;
}

jlong getActualClassTag(ClassModule * classModule, jlong classTag,
		jlong* tag_ptr) {
	if (isJClass(classModule, classTag)) {
		if (tag_ptr == NULL) {
			return 0;
		}
		return *tag_ptr;
	}
	return classTag;
}

