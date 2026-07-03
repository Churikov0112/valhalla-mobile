
#include <valhalla/worker.h>
#include "main.h"
#include "valhalla_actor.h"

#ifdef __ANDROID__
// The Android JNI interface uses a different function signature.
#include <jni.h>

extern "C"
JNIEXPORT jstring

JNICALL
Java_com_valhalla_valhalla_ValhallaKotlin_route(JNIEnv *env,
                                                jobject thiz,
                                                jstring jRequest,
                                                jstring jConfigPath) {
    
    const char *request = env->GetStringUTFChars(jRequest, 0);
    const char *config_path = env->GetStringUTFChars(jConfigPath, 0);

    std::string result;
    try {
        // TODO: Android currently creates a new actor every time. Optimize to be like iOS later.
        ValhallaActor valhallaActor(config_path);
        result = valhallaActor.route(request);
    } catch (const valhalla::valhalla_exception_t &err) {
        printf("[ValhallaActor] route valhalla_exception: %s\n", err.what());
        std::string code = std::to_string(err.code);
        std::string message = err.message.c_str();

        result = "{\"code\":" + code + ",\"message\":\"" + message + "\"}";
    } catch (const std::exception &err) {
        printf("[ValhallaActor] route std::exception: %s\n", err.what());
        result = "{\"code\":-1,\"message\":\"" + std::string(err.what()) + "\"}";
    } catch (...) {
        printf("[ValhallaActor] route unknown exception");
        result = "{\"code\":-1,\"message\":\"unknown exception\"}";
    }

    env->ReleaseStringUTFChars(jRequest, request);
    env->ReleaseStringUTFChars(jConfigPath, config_path);

    return env->NewStringUTF(result.c_str());
}

extern "C"
JNIEXPORT jstring

JNICALL
Java_com_valhalla_valhalla_ValhallaKotlin_optimizedRoute(JNIEnv *env,
                                                         jobject thiz,
                                                         jstring jRequest,
                                                         jstring jConfigPath) {
    
    const char *request = env->GetStringUTFChars(jRequest, 0);
    const char *config_path = env->GetStringUTFChars(jConfigPath, 0);

    std::string result;
    try {
        // TODO: Android currently creates a new actor every time. Optimize to be like iOS later.
        ValhallaActor valhallaActor(config_path);
        result = valhallaActor.optimized_route(request);
    } catch (const valhalla::valhalla_exception_t &err) {
        printf("[ValhallaActor] optimized_route valhalla_exception: %s\n", err.what());
        std::string code = std::to_string(err.code);
        std::string message = err.message.c_str();

        result = "{\"code\":" + code + ",\"message\":\"" + message + "\"}";
    } catch (const std::exception &err) {
        printf("[ValhallaActor] optimized_route std::exception: %s\n", err.what());
        result = "{\"code\":-1,\"message\":\"" + std::string(err.what()) + "\"}";
    } catch (...) {
        printf("[ValhallaActor] optimized_route unknown exception");
        result = "{\"code\":-1,\"message\":\"unknown exception\"}";
    }

    env->ReleaseStringUTFChars(jRequest, request);
    env->ReleaseStringUTFChars(jConfigPath, config_path);

    return env->NewStringUTF(result.c_str());
}

extern "C"
JNIEXPORT jstring

JNICALL
Java_com_valhalla_valhalla_ValhallaKotlin_height(JNIEnv *env,
                                                 jobject thiz,
                                                 jstring jRequest,
                                                 jstring jConfigPath) {

    const char *request = env->GetStringUTFChars(jRequest, 0);
    const char *config_path = env->GetStringUTFChars(jConfigPath, 0);

    std::string result;
    try {
        ValhallaActor valhallaActor(config_path);
        result = valhallaActor.height(request);
    } catch (const valhalla::valhalla_exception_t &err) {
        printf("[ValhallaActor] height valhalla_exception: %s\n", err.what());
        std::string code = std::to_string(err.code);
        std::string message = err.message.c_str();

        result = "{\"code\":" + code + ",\"message\":\"" + message + "\"}";
    } catch (const std::exception &err) {
        printf("[ValhallaActor] height std::exception: %s\n", err.what());
        result = "{\"code\":-1,\"message\":\"" + std::string(err.what()) + "\"}";
    } catch (...) {
        printf("[ValhallaActor] height unknown exception");
        result = "{\"code\":-1,\"message\":\"unknown exception\"}";
    }

    env->ReleaseStringUTFChars(jRequest, request);
    env->ReleaseStringUTFChars(jConfigPath, config_path);

    return env->NewStringUTF(result.c_str());
}

extern "C"
JNIEXPORT jstring

JNICALL
Java_com_valhalla_valhalla_ValhallaKotlin_locate(JNIEnv *env,
                                                  jobject thiz,
                                                  jstring jRequest,
                                                  jstring jConfigPath) {

    const char *request = env->GetStringUTFChars(jRequest, 0);
    const char *config_path = env->GetStringUTFChars(jConfigPath, 0);

    std::string result;
    try {
        ValhallaActor valhallaActor(config_path);
        result = valhallaActor.locate(request);
    } catch (const valhalla::valhalla_exception_t &err) {
        printf("[ValhallaActor] locate valhalla_exception: %s\n", err.what());
        std::string code = std::to_string(err.code);
        std::string message = err.message.c_str();

        result = "{\"code\":" + code + ",\"message\":\"" + message + "\"}";
    } catch (const std::exception &err) {
        printf("[ValhallaActor] locate std::exception: %s\n", err.what());
        result = "{\"code\":-1,\"message\":\"" + std::string(err.what()) + "\"}";
    } catch (...) {
        printf("[ValhallaActor] locate unknown exception");
        result = "{\"code\":-1,\"message\":\"unknown exception\"}";
    }

    env->ReleaseStringUTFChars(jRequest, request);
    env->ReleaseStringUTFChars(jConfigPath, config_path);

    return env->NewStringUTF(result.c_str());
}

extern "C"
JNIEXPORT jstring

JNICALL
Java_com_valhalla_valhalla_ValhallaKotlin_traceRoute(JNIEnv *env,
                                                      jobject thiz,
                                                      jstring jRequest,
                                                      jstring jConfigPath) {

    const char *request = env->GetStringUTFChars(jRequest, 0);
    const char *config_path = env->GetStringUTFChars(jConfigPath, 0);

    std::string result;
    try {
        ValhallaActor valhallaActor(config_path);
        result = valhallaActor.trace_route(request);
    } catch (const valhalla::valhalla_exception_t &err) {
        printf("[ValhallaActor] trace_route valhalla_exception: %s\n", err.what());
        std::string code = std::to_string(err.code);
        std::string message = err.message.c_str();

        result = "{\"code\":" + code + ",\"message\":\"" + message + "\"}";
    } catch (const std::exception &err) {
        printf("[ValhallaActor] trace_route std::exception: %s\n", err.what());
        result = "{\"code\":-1,\"message\":\"" + std::string(err.what()) + "\"}";
    } catch (...) {
        printf("[ValhallaActor] trace_route unknown exception");
        result = "{\"code\":-1,\"message\":\"unknown exception\"}";
    }

    env->ReleaseStringUTFChars(jRequest, request);
    env->ReleaseStringUTFChars(jConfigPath, config_path);

    return env->NewStringUTF(result.c_str());
}

#elif __APPLE__
void* create_valhalla_actor(const char *config_path, ValhallaMobileHttpClient* http_client) {
    return new ValhallaActor(config_path, http_client);
}

void delete_valhalla_actor(void* actor) {
    delete ((ValhallaActor*) actor);
}

std::string route(const char *request, void* actor) {
    std::string result;
    try {
        result = ((ValhallaActor*) actor)->route(request);
    } catch (const valhalla::valhalla_exception_t &err) {
        printf("[ValhallaActor] route valhalla_exception: %s\n", err.what());
        std::string code = std::to_string(err.code);
        std::string message = err.message.c_str();

        result = "{\"code\":" + code + ",\"message\":\"" + message + "\"}";
    } catch (const std::exception &err) {
        printf("[ValhallaActor] route std::exception: %s\n", err.what());
        result = "{\"code\":-1,\"message\":\"" + std::string(err.what()) + "\"}";
    } catch (...) {
        printf("[ValhallaActor] route unknown exception");
        result = "{\"code\":-1,\"message\":\"unknown exception\"}";
    }

    return result;
}

std::string optimized_route(const char *request, void* actor) {
    std::string result;
    try {
        result = ((ValhallaActor*) actor)->optimized_route(request);
    } catch (const valhalla::valhalla_exception_t &err) {
        printf("[ValhallaActor] optimized_route valhalla_exception: %s\n", err.what());
        std::string code = std::to_string(err.code);
        std::string message = err.message.c_str();

        result = "{\"code\":" + code + ",\"message\":\"" + message + "\"}";
    } catch (const std::exception &err) {
        printf("[ValhallaActor] optimized_route std::exception: %s\n", err.what());
        result = "{\"code\":-1,\"message\":\"" + std::string(err.what()) + "\"}";
    } catch (...) {
        printf("[ValhallaActor] optimized_route unknown exception");
        result = "{\"code\":-1,\"message\":\"unknown exception\"}";
    }

    return result;
}

std::string height(const char *request, void* actor) {
    std::string result;
    try {
        result = ((ValhallaActor*) actor)->height(request);
    } catch (const valhalla::valhalla_exception_t &err) {
        printf("[ValhallaActor] height valhalla_exception: %s\n", err.what());
        std::string code = std::to_string(err.code);
        std::string message = err.message.c_str();

        result = "{\"code\":" + code + ",\"message\":\"" + message + "\"}";
    } catch (const std::exception &err) {
        printf("[ValhallaActor] height std::exception: %s\n", err.what());
        result = "{\"code\":-1,\"message\":\"" + std::string(err.what()) + "\"}";
    } catch (...) {
        printf("[ValhallaActor] height unknown exception");
        result = "{\"code\":-1,\"message\":\"unknown exception\"}";
    }

    return result;
}

std::string locate(const char *request, void* actor) {
    std::string result;
    try {
        result = ((ValhallaActor*) actor)->locate(request);
    } catch (const valhalla::valhalla_exception_t &err) {
        printf("[ValhallaActor] locate valhalla_exception: %s\n", err.what());
        std::string code = std::to_string(err.code);
        std::string message = err.message.c_str();

        result = "{\"code\":" + code + ",\"message\":\"" + message + "\"}";
    } catch (const std::exception &err) {
        printf("[ValhallaActor] locate std::exception: %s\n", err.what());
        result = "{\"code\":-1,\"message\":\"" + std::string(err.what()) + "\"}";
    } catch (...) {
        printf("[ValhallaActor] locate unknown exception");
        result = "{\"code\":-1,\"message\":\"unknown exception\"}";
    }

    return result;
}

std::string trace_route(const char *request, void* actor) {
    std::string result;
    try {
        result = ((ValhallaActor*) actor)->trace_route(request);
    } catch (const valhalla::valhalla_exception_t &err) {
        printf("[ValhallaActor] trace_route valhalla_exception: %s\n", err.what());
        std::string code = std::to_string(err.code);
        std::string message = err.message.c_str();

        result = "{\"code\":" + code + ",\"message\":\"" + message + "\"}";
    } catch (const std::exception &err) {
        printf("[ValhallaActor] trace_route std::exception: %s\n", err.what());
        result = "{\"code\":-1,\"message\":\"" + std::string(err.what()) + "\"}";
    } catch (...) {
        printf("[ValhallaActor] trace_route unknown exception");
        result = "{\"code\":-1,\"message\":\"unknown exception\"}";
    }

    return result;
}
#endif
