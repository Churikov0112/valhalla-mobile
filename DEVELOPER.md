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

## Сборка офлайн-тайлов с транзитом (GTFS) для Санкт-Петербурга

Мультимодальный (пешеход + общественный транспорт) офлайн-роутинг требует особых
тайлов Valhalla с транзитным слоем (level 3), собранным из GTFS. Готовый
воспроизводимый скрипт — **`fkr_app/scripts/build_valhalla_tiles.sh`** (Docker).
Ниже — полный пайплайн и подводные камни, найденные при интеграции ОРГП СПб.

### Критично: версия Valhalla

Формат графовых тайлов версионный. **Тайлы обязаны собираться той же версией
Valhalla, что вкомпилирована в движок** (см. подмодуль `src/valhalla`). На момент
написания это **3.6.3** (коммит `e2f017b`). Собирать официальным образом
`ghcr.io/valhalla/valhalla:3.6.3` (на Apple Silicon — `:3.6.3-arm64`, подтверждён
в реестре), НЕ `:latest`. Тайлы от другой версии движок молча не читает как
транзит → маршрут «сваливается в пешку» или даёт `No path`.

### Порядок шагов (нельзя менять)

Транзит собирается ДО `build_tiles`, т.к. именно `valhalla_build_tiles`
подключает остановки к уличному графу:

1. `valhalla_build_timezones` → `tz_world.sqlite`
2. `valhalla_build_config` с `--mjolnir-transit-dir` и `--mjolnir-transit-feeds-dir`
3. `valhalla_build_admins -c valhalla.json spb.osm.pbf`
4. `valhalla_ingest_transit -c valhalla.json`  (GTFS → промежуточный protobuf)
5. `valhalla_convert_transit -c valhalla.json`  (protobuf → графовые тайлы level 3)
6. `valhalla_build_tiles -c valhalla.json spb.osm.pbf`  (улицы 0–2 + связка с транзитом)

Устаревший `valhalla_build_transit` не использовать — его нет в 3.6.x.

### Входные данные

- **OSM:** Geofabrik, датированный снапшот `russia/northwestern-fed-district-YYMMDD.osm.pbf`
  (пиннинг + проверка `.md5`, НЕ `-latest`), обрезка по bbox СПб
  `29.879734,59.617364,30.837342,60.163080` (`min_lon,min_lat,max_lon,max_lat`).
  **osmium в образе Valhalla отсутствует** — обрезка отдельным образом
  `stefda/osmium-tool` (amd64 под эмуляцией). Экстракт обязан покрывать и все
  остановки GTFS, и целевые точки маршрута.
- **GTFS ОРГП:** реальный файл — `https://transport.orgp.spb.ru/Portal/transport/internalapi/gtfs/feed.zip`
  (сам `.../gtfs` — HTML-индекс). Нужен `curl -k`: сервер использует «Russian
  Trusted Sub CA», которого нет в системном bundle. `.txt` лежат в корне архива.

### Подводный камень №1: инвертированный `calendar.txt` ОРГП

У ОРГП в `calendar.txt` **все диапазоны инвертированы** (`start_date > end_date`).
Из-за этого Valhalla считает ВСЕ сервисы недействительными на любую дату → транзит
не строится ни разу (`No path`), даже когда level-3 тайлы физически есть.
Реальные корректировки по датам — в `calendar_dates.txt` (`exception_type` 1/2).
Скрипт переписывает диапазоны в валидные, покрывающие целевые дни, и чинит флаги
дней недели. **Без этого фикса транзит не работает вообще.**

### Подводный камень №2: `frequencies.txt` → OOM. ПОЧЕМУ ТОЛЬКО 2 ЧАСА ДАННЫХ

> Это ответ на вопрос «почему в демо-тайлах всего ~2 часа расписания».

Полный фид ОРГП использует `frequencies.txt` (интервальное движение). При
разворачивании в конкретные отправления это **~950 000 рейсовых отправлений** на
сутки. `valhalla_convert_transit` на плотных центральных тайлах при этом уходит в
**OOM (exit 137)** и работает часами на один тайл.

Поэтому текущие демо-тайлы, лежащие в `fkr_app/assets/map/tiles/valhalla`,
намеренно сужены до:
- **одного служебного дня — 2026-06-12** (внимание: это День России, расписание
  аномальное — исторически так вышло, что автоподбор «плотного дня» дал именно
  его; для чистого демо это некритично);
- **окна времени 08:00–10:00** — т.е. всего ~2 часа. Дефолтное время отправления
  в приложении — 09:00, оно попадает в это окно.

Так собирается за минуты и влезает в ~4.8 ГБ памяти Docker. Это **демо**, а не
полное покрытие: на других датах/во внерабочее время транзита в этих тайлах нет.

### Как собрать полноценные тайлы (и цена)

Параметры скрипта (env):
- `SERVICE_TUE` / `SERVICE_SUN` — целевые служебные дни (для двух репрезентативных
  типов дня задать РАЗНЫЕ, напр. нормальный вторник `2026-06-23` и воскресенье
  `2026-06-21`; для одного дня — оба одинаковые).
- `BAND_LO` / `BAND_HI` — окно времени. Демо: `08:00:00`..`10:00:00`. Полные сутки
  с ночными рейсами: `00:00:00`..`27:00:00`.
- `OSM_DATE` — дата снапшота Geofabrik (`YYMMDD`).

Ориентиры для полного покрытия (экстраполяция, не бенчмарк): полные сутки ×
несколько дней — часы работы `convert_transit`, **16–32 ГБ RAM** в Docker,
level-3 разрастается до сотен МБ (для мобильного приложения это заметный вес
ассетов). Для параллелизма поднять `concurrency` в `valhalla.json` — но это
кратно увеличивает пик памяти.

### Проверка (Docker, без устройства)

```bash
IMG=ghcr.io/valhalla/valhalla:3.6.3-arm64
docker run --rm -v "$PWD/build/valhalla_tiles:/data/tiles" "$IMG" \
  valhalla_build_config --mjolnir-tile-dir /data/tiles \
  --mjolnir-timezone /data/tiles/tz_world.sqlite --mjolnir-admin /data/tiles/admins.sqlite \
  > /tmp/v.json
docker run -d --name v -p 8002:8002 -v "$PWD/build/valhalla_tiles:/data/tiles" \
  -v /tmp:/work "$IMG" valhalla_service /work/v.json 1
# запрос: costing=multimodal + date_time внутри окна (тип 1). Ждём travel_mode=transit.
curl -s localhost:8002/route --data '{"locations":[{"lat":59.8749,"lon":30.3220},
  {"lat":59.9391,"lon":30.3158}],"costing":"multimodal",
  "date_time":{"type":1,"value":"2026-06-12T09:00"}}'
docker rm -f v
```

### Подводный камень №3: мультимодал с via-точками

Один запрос `costing=multimodal` с **3+ локациями** (промежуточные via) при
фиксированном `date_time` даёт `No path` (ограничение движка). **Попарные** леги
(pt1→pt2, pt2→pt3) строятся с транзитом. Маршрут через много точек надо собирать
по-легово, а не одним запросом.

### Интеграция в fkr_app

- Скопировать `build/valhalla_tiles/{0,1,2,3,admins.sqlite,tz_world.sqlite}` в
  `assets/map/tiles/valhalla/` (транзит зашит в level-3 `.gph`; промежуточный
  `transit/` не нужен; DEM `elevation/` не трогать).
- В `pubspec.yaml` перечислить каждый листовой каталог тайлов явно (Flutter не
  рекурсирует в подпапки).
- Поднять `_configVersion` в `ValhallaTileManager` — иначе устройство не
  перекопирует тайлы.
- Запрос `route` слать с `date_time` (тип 1, `<день>T09:00`) — без него
  мультимодал не планирует по расписанию.

