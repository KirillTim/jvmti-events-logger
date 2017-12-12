#include <stdio.h>

#include <string>

#include <jni.h>
#include <jvmti.h>
#include <jvmticmlr.h>
#include <memory.h>
#include <iostream>

using namespace std;

const int STRING_BUFFER_SIZE = 1000;

string class_name_from_sig(const string &sig) {
    string result = string(sig);
    if (result[0] == 'L') result = result.substr(1);
    result = result.substr(0, result.find(';'));
    for (char &i : result) {
        if (i == '/') i = '.';
    }
    return result;
}

static string sig_string(jvmtiEnv *jvmti, jmethodID method) {
    char *generic_method_sig = nullptr;
    char *generic_class_sig = nullptr;
    char *method_name = nullptr;
    char *msig = nullptr;
    char *csig = nullptr;
    jclass jcls;
    string result;
    if (!jvmti->GetMethodName(method, &method_name, &msig, &generic_method_sig)) {
        if (!jvmti->GetMethodDeclaringClass(method, &jcls)
            && !jvmti->GetClassSignature(jcls, &csig, &generic_class_sig)) {
            result = string(class_name_from_sig(csig)) + "." + string(method_name);
            //if (generic_class_sig != nullptr) result += " generic_class_sig = " + string(generic_class_sig);
        }
    }
    if (generic_method_sig != nullptr) jvmti->Deallocate(reinterpret_cast<unsigned char *>(generic_method_sig));
    if (generic_class_sig != nullptr) jvmti->Deallocate(reinterpret_cast<unsigned char *>(generic_class_sig));
    if (method_name != nullptr) jvmti->Deallocate(reinterpret_cast<unsigned char *>(method_name));
    if (csig != nullptr) jvmti->Deallocate(reinterpret_cast<unsigned char *>(csig));
    if (msig != nullptr) jvmti->Deallocate(reinterpret_cast<unsigned char *>(msig));
    return result;
}

void set_notification_mode(jvmtiEnv *jvmti) {
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_COMPILED_METHOD_LOAD, nullptr);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_COMPILED_METHOD_UNLOAD, nullptr);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_DYNAMIC_CODE_GENERATED, nullptr);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, nullptr);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION_CATCH, nullptr);
}

static void JNICALL
cbCompiledMethodLoad(
        jvmtiEnv *jvmti,
        jmethodID method,
        jint code_size,
        const void *code_addr,
        jint map_length,
        const jvmtiAddrLocationMap *map,
        const void *compile_info) {
    cout << "cbCompiledMethodLoad: " << sig_string(jvmti, method) << endl;
}

void JNICALL
cbCompiledMethodUnload(jvmtiEnv *jvmti,
                       jmethodID method,
                       const void *code_addr) {
    cout << "cbCompiledMethodUnload: " << sig_string(jvmti, method) << endl;
}

void JNICALL
cbDynamicCodeGenerated(jvmtiEnv *jvmti,
                       const char *name,
                       const void *address,
                       jint length) {
    //printf("cbDynamicCodeGenerated %s %lx\n", name, (unsigned long) address);
}

void JNICALL
cbException(jvmtiEnv *jvmti_env,
            JNIEnv *jni_env,
            jthread thread,
            jmethodID method,
            jlocation location,
            jobject exception,
            jmethodID catch_method,
            jlocation catch_location) {
    //cout << "cbException: from " << sig_string(jvmti_env, method) << endl;
}

void JNICALL
cbExceptionCatch(jvmtiEnv *jvmti_env,
                 JNIEnv *jni_env,
                 jthread thread,
                 jmethodID method,
                 jlocation location,
                 jobject exception) {
    //printf("cbExceptionCatch\n");
}

jvmtiError set_callbacks(jvmtiEnv *jvmti) {
    jvmtiEventCallbacks callbacks;

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.CompiledMethodLoad = &cbCompiledMethodLoad;
    callbacks.CompiledMethodUnload = &cbCompiledMethodUnload;
    callbacks.DynamicCodeGenerated = &cbDynamicCodeGenerated;
    callbacks.Exception = &cbException;
    callbacks.ExceptionCatch = &cbExceptionCatch;
    return jvmti->SetEventCallbacks(&callbacks, (jint) sizeof(callbacks));
}

jvmtiError enable_capabilities(jvmtiEnv *jvmti) {
    jvmtiCapabilities capabilities;
    memset(&capabilities, 0, sizeof(capabilities));
    capabilities.can_generate_compiled_method_load_events = 1;
    capabilities.can_generate_exception_events = 1;
    // Request these capabilities for this JVM TI environment.
    return jvmti->AddCapabilities(&capabilities);
}

static void agent_main(JavaVM *vm, char *options, void *reserved) {
    cout << "agent_main" << endl;
    jvmtiEnv *jvmti;
    vm->GetEnv((void **) &jvmti, JVMTI_VERSION_1);
    enable_capabilities(jvmti);
    set_callbacks(jvmti);
    set_notification_mode(jvmti);
    jvmti->GenerateEvents(JVMTI_EVENT_DYNAMIC_CODE_GENERATED);
    jvmti->GenerateEvents(JVMTI_EVENT_COMPILED_METHOD_LOAD);
    jvmti->GenerateEvents(JVMTI_EVENT_COMPILED_METHOD_UNLOAD);
}


JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("Agent_OnLoad\n");
    agent_main(vm, options, reserved);
    return 0;
}

JNIEXPORT jint JNICALL
Agent_OnAttach(JavaVM *vm, char *options, void *reserved) {
    printf("Agent_OnAttach\n");
    agent_main(vm, options, reserved);
    return 0;
}
