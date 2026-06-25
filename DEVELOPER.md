# Developer Guide

## Architecture

```
┌──────────────────────────────────────────────┐
│  Flutter App (Dart)                           │
│  ValhallaService (MethodChannel)              │
├──────────────────────────────────────────────┤
│  Platform Bridge                              │
│  ┌─────────────────┐ ┌─────────────────────┐ │
│  │ iOS (Swift)      │ │ Android (Kotlin)    │ │
│  │ Valhalla.swift   │ │ ValhallaKotlin.kt   │ │
│  │ ValhallaWrapper  │ │ Valhalla.kt         │ │
│  └──────┬──────────┘ └──────┬──────────────┘ │
│         │ ObjC              │ JNI             │
│         ▼                    ▼                 │
│  ┌──────────────────────────────────────────┐ │
│  │  C++ (src/wrapper/)                      │ │
│  │  main.cpp + valhalla_actor.cpp           │ │
│  │  ValhallaActor::route()                  │ │
│  │  ValhallaActor::optimized_route()        │ │
│  └──────────────┬───────────────────────────┘ │
│                 ▼                              │
│  ┌──────────────────────────────────────────┐ │
│  │  valhalla/tyr/actor_t                     │ │
│  │  (Valhalla routing engine submodule)      │ │
│  └──────────────────────────────────────────┘ │
└──────────────────────────────────────────────┘
```

### Layers

| Layer | iOS | Android |
|-------|-----|---------|
| Dart | `ValhallaService` (MethodChannel `valhalla_channel`) | same |
| Native bridge | `Valhalla.swift` → `ValhallaWrapper.mm` (ObjC) | `ValhallaKotlin.kt` → JNI |
| C++ wrapper | `main.cpp` (iOS C bindings) | `main.cpp` (Android JNI) |
| C++ actor | `valhalla_actor.cpp` — `ValhallaActor` class | same |
| Engine | Valhalla `tyr::actor_t` | same |

## How to Add a New Method

Example: adding method `my_method`.

### 1. C++ — `valhalla_actor.h` / `valhalla_actor.cpp`

```cpp
// valhalla_actor.h
std::string my_method(const std::string& request);

// valhalla_actor.cpp
std::string ValhallaActor::my_method(const std::string& request) {
    std::string req = std::string(request);
    std::string result = actor->my_method(req);  // calls valhalla::tyr::actor_t
    return result;
}
```

### 2. C++ — `main.h`

```cpp
std::string my_method(const std::string& request, const std::string& config_path);
```

### 3. iOS — `main.cpp` (C bindings for ObjC)

```cpp
// In the iOS section (#ifndef __ANDROID__)
extern "C" {
    const char* my_method(const char* request, const char* config_path) {
        ValhallaActor actor(config_path);
        std::string result = actor.my_method(request);
        return strdup(result.c_str());
    }
}
```

### 4. iOS — `ValhallaWrapper.h` (ObjC)

```objc
- (NSString*)myMethod:(NSString*)request;
```

### 5. iOS — `ValhallaWrapper.mm` (ObjC)

```objc
- (NSString*)myMethod:(NSString*)request {
    const char* result = my_method([request UTF8String], [self.configPath UTF8String]);
    return [NSString stringWithUTF8String:result];
}
```

### 6. iOS — `Valhalla.swift`

```swift
public protocol ValhallaProviding {
    func myMethod(request: RouteRequest) throws -> RouteResponse
}

public func myMethod(request: RouteRequest) throws -> RouteResponse {
    let requestData = try JSONEncoder().encode(request)
    guard let requestStr = String(data: requestData, encoding: .utf8) else {
        throw ValhallaError.encodingNotUtf8("requestStr")
    }
    let resultStr = actor.myMethod(requestStr)
    // decode + error handling ...
}
```

### 7. Android — `main.cpp` (JNI)

```cpp
// In the Android section (#ifdef __ANDROID__)
extern "C" JNIEXPORT jstring JNICALL
Java_com_valhalla_valhalla_ValhallaKotlin_myMethod(JNIEnv* env, jobject thiz, jstring jRequest, jstring jConfigPath) {
    const char* request = env->GetStringUTFChars(jRequest, 0);
    const char* config_path = env->GetStringUTFChars(jConfigPath, 0);
    ValhallaActor valhallaActor(config_path);
    std::string result = valhallaActor.my_method(request);
    env->ReleaseStringUTFChars(jRequest, request);
    env->ReleaseStringUTFChars(jConfigPath, config_path);
    return env->NewStringUTF(result.c_str());
}
```

### 8. Android — `Valhalla.kt`

```kotlin
fun myMethod(request: String): String {
    return ValhallaKotlin.myMethod(request, configPath)
}
```

### 9. Android — `ValhallaActor.kt` / `ValhallaKotlin.kt`

```kotlin
external fun myMethod(request: String, configPath: String): String
```

### 10. Dart — `ValhallaService`

```dart
Future<Map<String, dynamic>> myMethod({
    required String costing,
    required List<Map<String, double>> waypoints,
}) async {
    final result = await _channel.invokeMethod('myMethod', {
        'costing': costing,
        'waypoints': waypoints,
    });
    return jsonDecode(result as String) as Map<String, dynamic>;
}
```

### 11. iOS — `AppDelegate.swift` (Flutter bridge)

```swift
bridge.myMethod(request: requestString) { result, error in
    result?(result)
}
```

### 12. Rebuild

- **iOS**: `./build.sh ios clean` or per-architecture scripts in `scripts/`
- **Android**: `./build.sh android clean` or let Gradle build automatically

## How to Integrate in a Flutter Project

### iOS — Swift Package Manager

In Xcode: add package dependency via `XCRemoteSwiftPackageReference`:

```
https://github.com/Churikov0112/valhalla-mobile.git
Version: 0.5.3, up to next major
```

Or in `Package.swift`:

```swift
.package(url: "https://github.com/Churikov0112/valhalla-mobile.git", from: "0.5.3")
```

> The XCFramework is downloaded automatically from the GitHub Release as a binary target.
> Set `VALHALLA_MOBILE_DEV=true` to use a local XCFramework from `build/apple/`.

### Android — GitHub Packages

In `android/build.gradle.kts`:

```kotlin
allprojects {
    repositories {
        google()
        mavenCentral()
        maven {
            name = "GitHubPackages"
            url = uri("https://maven.pkg.github.com/Churikov0112/valhalla-mobile")
            credentials {
                username = project.findProperty("gpr.user") as String?
                    ?: System.getenv("GITHUB_ACTOR")
                password = project.findProperty("gpr.key") as String?
                    ?: System.getenv("GITHUB_TOKEN")
            }
        }
    }
}
```

In `android/app/build.gradle.kts`:

```kotlin
dependencies {
    implementation("com.github.churikov0112:valhalla-mobile:0.5.3")
}
```

**Token**: Developers need a classic GitHub PAT with `read:packages` scope.
Add to `~/.gradle/gradle.properties`:

```properties
gpr.key=ghp_...
gpr.user=Churikov0112
```

CI (GitHub Actions) uses the built-in `GITHUB_TOKEN` automatically.
