# Developer Guide

## Architecture

```
┌──────────────────────────────────────────────────┐
│  Flutter App (Dart)                               │
│  ValhallaService                                  │
│  └─ строит Valhalla JSON                          │
│  └─ разбирает ответ                               │
│  └─ MethodChannel('valhalla_channel')              │
├──────────────────────────────────────────────────┤
│  Platform Bridge (чистые прокси, pass-through)     │
│  ┌──────────────────┐ ┌────────────────────────┐ │
│  │ iOS (Swift)       │ │ Android (Kotlin)       │ │
│  │ AppDelegate       │ │ MainActivity.kt        │ │
│  │ → Valhalla.swift  │ │ → ValhallaKotlin.kt    │ │
│  │ → ValhallaWrapper │ │ → JNI                  │ │
│  │   (ObjC)          │ │                        │ │
│  └──────┬───────────┘ └──────┬─────────────────┘ │
│         │ ObjC               │ JNI                │
│         ▼                     ▼                   │
│  ┌──────────────────────────────────────────────┐ │
│  │  C++ (src/wrapper/)                          │ │
│  │  main.cpp + valhalla_actor.cpp               │ │
│  │  ValhallaActor::route()                      │ │
│  │  ValhallaActor::optimized_route()            │ │
│  └──────────────────┬───────────────────────────┘ │
│                     ▼                              │
│  ┌──────────────────────────────────────────────┐ │
│  │  valhalla/tyr/actor_t                         │ │
│  │  (Valhalla routing engine submodule)          │ │
│  └──────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────┘
```

### Layers

| Layer | iOS | Android |
|-------|-----|---------|
| Dart | `ValhallaService` — builds Valhalla JSON, calls MethodChannel `{'request': jsonString}` | same |
| Native bridge (proxy) | `Valhalla.swift` → `ValhallaWrapper.mm` (ObjC) — pass-through raw JSON | `ValhallaKotlin.kt` → JNI — pass-through raw JSON |
| C++ wrapper | `main.cpp` (iOS C bindings) | `main.cpp` (Android JNI) |
| C++ actor | `valhalla_actor.cpp` — `ValhallaActor` class | same |
| Engine | Valhalla `tyr::actor_t` | same |

> Native layer (Swift/Kotlin) is a pure proxy — it does not construct or parse Valhalla JSON.
> All request building lives in Dart's `ValhallaService`.

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
    // Build Valhalla JSON request in Dart, send as raw string
    final requestJson = jsonEncode({
        'locations': waypoints.map((w) => {'lon': w['lon'], 'lat': w['lat']}).toList(),
        'costing': costing,
    });
    final result = await _channel.invokeMethod('myMethod', {
        'request': requestJson,
    });
    return jsonDecode(result as String) as Map<String, dynamic>;
}
```

> The native layer (iOS/Android) receives the raw JSON string and passes it directly to C++.
> JSON construction lives entirely in Dart.

### 11. iOS — `AppDelegate.swift` (Flutter bridge)

```swift
// Add "myMethod" to the existing group case (shared raw-string protocol):
case "route", "optimizedRoute", "height", "locate", "myMethod":
    guard let request = args["request"] as? String else {
        result(FlutterError(code: "INVALID_ARGS", ...))
        return
    }
    let response: String
    switch call.method {
    case "route": response = bridge.route(request: request)
    case "myMethod": response = bridge.myMethod(request: request)
    default: fatalError()
    }
    result(response)
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

## Full Pipeline: Development → Test → Release

### Overview

```
Add C++ method → Add iOS/Android/Dart layers → Local build → Local test → Bump version → Build + publish → Update fkr_app
```

---

### 1. Environment Setup

```bash
# iOS
xcode-select --install                # Xcode CLI tools
brew install cmake ninja              # Build tools

# Android
brew install openjdk@17              # JDK 17
export JAVA_HOME=/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home

# Android NDK (via SDK Manager or manual download)
export ANDROID_NDK_HOME=~/Library/Android/sdk/ndk/28.2.13676358

# vcpkg (must be cloned in repo root)
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT="$(pwd)/vcpkg"

# GitHub Packages token (classic PAT with write:packages)
# Add to ~/.gradle/gradle.properties:
#   gpr.user=Churikov0112
#   gpr.key=ghp_xxx
```

### 2. Add a New Method

See [How to Add a New Method](#how-to-add-a-new-method) above.

Files to touch (in order):

| # | File | What |
|---|------|------|
| 1 | `src/wrapper/include/valhalla_actor.h` | Declare method on `ValhallaActor` |
| 2 | `src/wrapper/valhalla_actor.cpp` | Implement — call `actor->my_method(req)` |
| 3 | `src/wrapper/include/main.h` | Declare C binding (iOS) + JNI (Android) |
| 4 | `src/wrapper/main.cpp` | Implement C binding + JNI with error handling |
| 5 | `apple/Sources/ValhallaObjc/include/ValhallaWrapper.h` | ObjC interface |
| 6 | `apple/Sources/ValhallaObjc/ValhallaWrapper.mm` | ObjC → C bridge |
| 7 | `apple/Sources/Valhalla/Valhalla.swift` | Swift method + protocol conformance |
| 8 | `android/valhalla/…/ValhallaKotlin.kt` | `external fun` declaration |
| 9 | `android/valhalla/…/ValhallaActor.kt` | Interface + implementation |
| 10 | `android/valhalla/…/Valhalla.kt` | Public API with JSON serialization |

### 3. Local Build

```bash
# From repo root
cd ~/Desktop/projects/valhalla-mobile

# iOS — builds 3 archs + creates XCFramework
bash build.sh ios
# → build/apple/valhalla-wrapper.xcframework

# Android — builds 4 archs + moves .so to jniLibs
export JAVA_HOME=/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home
export ANDROID_NDK_HOME=~/Library/Android/sdk/ndk/28.2.13676358
export VCPKG_ROOT="$(pwd)/vcpkg"

bash build.sh android

# Build AAR
cd android && ./gradlew :valhalla:assembleRelease
cd ..
# → android/valhalla/build/outputs/aar/valhalla-release.aar
```

> **Note**: If `bash build.sh android` fails, clean individual arch dirs and retry:
> ```bash
> rm -rf build/android/ARM_ARCH  # e.g. build/android/x86
> bash build.sh android
> ```

### 4. Local Integration in fkr_app

#### iOS — Local SPM Package

In `ios/Runner.xcodeproj/project.pbxproj`, replace remote with local:

```diff
- /* Begin XCRemoteSwiftPackageReference section */
-     repositoryURL = "https://github.com/Churikov0112/valhalla-mobile.git";
-     requirement = { kind = upToNextMajorVersion; minimumVersion = 0.5.3; };
- /* End XCRemoteSwiftPackageReference section */
+ /* Begin XCLocalSwiftPackageReference section */
+     isa = XCLocalSwiftPackageReference;
+     relativePath = "../../valhalla-mobile";
+ /* End XCLocalSwiftPackageReference section */
```

Then in Xcode: `File → Packages → Resolve Package Versions` (or clear `Package.resolved`).

#### Android — Local AAR

```bash
# Copy AAR
cp ~/Desktop/projects/valhalla-mobile/android/valhalla/build/outputs/aar/valhalla-release.aar \
   ~/Desktop/projects/fkr_app/android/app/libs/

# In android/app/build.gradle.kts:
#   implementation(fileTree("libs") { include("*.aar") })
# Remove the remote GitHub Packages line.
```

#### Flutter Bridge

Update these files in fkr_app:

| File | What |
|------|------|
| `lib/src/services/valhalla/valhalla_service.dart` | Add Dart method — builds Valhalla JSON, calls `_channel.invokeMethod('method', {'request': jsonString})` |
| `ios/Runner/ValhallaBridge.swift` | Add Swift bridge method — `func method(request: String) -> String` (raw string pass-through) |
| `ios/Runner/AppDelegate.swift` | Add `"methodName"` to the existing group case |
| `android/…/MainActivity.kt` | Add `"methodName"` to `when` + `ensureValhallaLoaded` reflection — uses `handleGeneric()` |

### 5. Release

#### 5.1 Bump Version

```bash
# version.txt — single source of truth
echo "0.5.4" > version.txt

# Package.swift — update version string
sed -i '' 's/let version: String = "0.5.3"/let version: String = "0.5.4"/' Package.swift
```

#### 5.2 Build XCFramework + Compute Checksum

```bash
bash build.sh ios

cd build/apple
zip -r -q --symlinks valhalla-wrapper.xcframework.zip valhalla-wrapper.xcframework
shasum -a 256 valhalla-wrapper.xcframework.zip | cut -d' ' -f1
# → copy this checksum

# Update Package.swift with new checksum
sed -i '' 's/let binaryChecksum: String = "OLD"/let binaryChecksum: String = "NEW"/' Package.swift
```

#### 5.3 Build AAR

```bash
export JAVA_HOME=/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home
export ANDROID_NDK_HOME=~/Library/Android/sdk/ndk/28.2.13676358
bash build.sh android
cd android && ./gradlew :valhalla:assembleRelease && cd ..
```

#### 5.4 Git Commit & Tag

```bash
git add -A
git commit -m "feat: add my_method (v0.5.4)"
git tag v0.5.4
git push origin feat/optimized-route
git push origin v0.5.4
```

> ⚠️ **Не меняй уже опубликованный тег.**
> 
> Если ты создал тег и GitHub Release — не делай `git commit --amend` и не пересоздавай тег (`git tag -d + git push --delete + git tag`). SPM запоминает коммит по хешу в `Package.resolved`. Если тег теперь указывает на другой коммит — резолв сломается, и придётся bumpить версию.
> 
> **Что делать, если нужно исправить `Package.swift` после релиза:**
> - Не трогай старый тег. Выпусти новую минорную версию (напр. `v0.5.5`).
> - Поменяй только `version.txt` и `Package.swift` (версия + checksum). Если бинарник не менялся — checksum тот же.
> - Создай новый тег + GitHub Release с тем же `.xcframework.zip`.

#### 5.5 GitHub Release (iOS)

1. Go to `https://github.com/Churikov0112/valhalla-mobile/releases`
2. Create release from tag `v0.5.4`
3. Upload `build/apple/valhalla-wrapper.xcframework.zip`
4. Publish

#### 5.6 Publish AAR to GitHub Packages

```bash
cd android
export JAVA_HOME=/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home
./gradlew :valhalla:publishMavenPublicationToGitHubPackagesRepository
```

> Requires `gpr.key` in `~/.gradle/gradle.properties` with `write:packages` scope.

#### 5.7 Switch fkr_app Between Local and Remote

> `android/build.gradle.kts` (project-level) already has GitHub Packages repository configured — no changes needed there.

**Switch to Local** (development on valhalla-mobile fork):

```bash
# Android — copy local AAR and add fileTree dependency
cp /path/to/valhalla-mobile/android/valhalla/build/outputs/aar/valhalla-release.aar \
   android/app/libs/valhalla-release.aar

# In android/app/build.gradle.kts, replace:
#   implementation("com.github.churikov0112:valhalla-mobile:0.5.6")
# with:
#   implementation(fileTree("libs") { include("*.aar") })

# iOS — replace remote SPM with local reference in project.pbxproj:
#   XCLocalSwiftPackageReference
#   relativePath = "../../valhalla-mobile"
```

**Switch Back to Remote** (release):

```bash
# Android — remove local AAR and restore remote dependency
rm android/app/libs/valhalla-release.aar

# In android/app/build.gradle.kts, replace:
#   implementation(fileTree("libs") { include("*.aar") })
# with:
#   implementation("com.github.churikov0112:valhalla-mobile:0.5.6")

# iOS — replace local SPM with remote reference in project.pbxproj:
#   XCRemoteSwiftPackageReference
#   repositoryURL = "https://github.com/Churikov0112/valhalla-mobile"
#   requirement = { kind = exactVersion; version = 0.5.6; }
```

### 6. Troubleshooting

#### `No space left on device` during Android build

```bash
# Clean cmake build dirs of already-built archs to free ~5 GB
rm -rf build/android/arm64-v8a build/android/armeabi-v7a build/android/x86_64

# iOS builds also take ~4 GB — delete if not needed
rm -rf build/apple
```

#### `Duplicate class` on Android (local + remote AAR both present)

Remove the remote `implementation(...)` line, keep only `fileTree(...)`.

#### `VALHALLA_MOBILE_DEV=true` with remote SPM doesn't work

This is expected — the local `build/apple/` path in `Package.swift` resolves relative to the SPM checkout in DerivedData, not your working copy. Use `XCLocalSwiftPackageReference` instead.

#### JNI function not found (`No implementation found for ...`)

The `.so` inside the AAR was built from old code. Rebuild:
```bash
rm -rf build/android
bash build.sh android
cd android && ./gradlew :valhalla:assembleRelease
```

Otherwise, ensure `main.cpp` declares the JNI function in the `__ANDROID__` section. The JNI function name must match the Kotlin class + method exactly.

#### `Revision X does not match previously recorded value Y`

SPM запомнил хеш коммита из `Package.resolved`, но тег на GitHub теперь ведёт на другой коммит (после force-push). 

**Решение:** удалить `Package.resolved` и кеш SPM:
```bash
rm -rf ~/Desktop/projects/fkr_app/ios/Runner.xcworkspace/xcshareddata/swiftpm
rm -f ~/Desktop/projects/fkr_app/ios/Runner.xcodeproj/project.xcworkspace/xcshareddata/swiftpm/Package.resolved
rm -rf ~/Library/Caches/org.swift.swiftpm
rm -rf ~/Library/Developer/Xcode/DerivedData/Runner-*
```

Если не помогает — выпустить новую версию (v0.5.5, v0.5.6, ...) и обновить `minimumVersion` в `project.pbxproj`.

