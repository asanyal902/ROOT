/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class XrdClientAdminJNI */

#ifndef _Included_XrdClientAdminJNI
#define _Included_XrdClientAdminJNI
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     XrdClientAdminJNI
 * Method:    chmod
 * Signature: (Ljava/lang/String;III)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_chmod
  (JNIEnv *, jobject, jstring, jint, jint, jint);

/*
 * Class:     XrdClientAdminJNI
 * Method:    dirlist
 * Signature: (Ljava/lang/String;[Ljava/lang/String;)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_dirlist
  (JNIEnv *, jobject, jstring, jobjectArray);

/*
 * Class:     XrdClientAdminJNI
 * Method:    existfiles
 * Signature: ([Ljava/lang/String;[Ljava/lang/Boolean;)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_existfiles
  (JNIEnv *, jobject, jobjectArray, jobjectArray);

/*
 * Class:     XrdClientAdminJNI
 * Method:    existdirs
 * Signature: ([Ljava/lang/String;[Ljava/lang/Boolean;)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_existdirs
  (JNIEnv *, jobject, jobjectArray, jobjectArray);

/*
 * Class:     XrdClientAdminJNI
 * Method:    getchecksum
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_getchecksum
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     XrdClientAdminJNI
 * Method:    isfileonline
 * Signature: ([Ljava/lang/String;[Ljava/lang/Boolean;)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_isfileonline
  (JNIEnv *, jobject, jobjectArray, jobjectArray);

/*
 * Class:     XrdClientAdminJNI
 * Method:    locate
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_locate
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     XrdClientAdminJNI
 * Method:    mv
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_mv
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     XrdClientAdminJNI
 * Method:    mkdir
 * Signature: (Ljava/lang/String;III)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_mkdir
  (JNIEnv *, jobject, jstring, jint, jint, jint);

/*
 * Class:     XrdClientAdminJNI
 * Method:    rm
 * Signature: (Ljava/lang/String;)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_rm
  (JNIEnv *, jobject, jstring);

/*
 * Class:     XrdClientAdminJNI
 * Method:    rmdir
 * Signature: (Ljava/lang/String;)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_rmdir
  (JNIEnv *, jobject, jstring);

/*
 * Class:     XrdClientAdminJNI
 * Method:    prepare
 * Signature: ([Ljava/lang/String;CC)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_prepare
  (JNIEnv *, jobject, jobjectArray, jchar, jchar);

/*
 * Class:     XrdClientAdminJNI
 * Method:    stat
 * Signature: (Ljava/lang/String;IJII)Ljava/lang/Boolean;
 */
JNIEXPORT jobject JNICALL Java_XrdClientAdminJNI_stat
  (JNIEnv *, jobject, jstring, jint, jlong, jint, jint);

#ifdef __cplusplus
}
#endif
#endif
