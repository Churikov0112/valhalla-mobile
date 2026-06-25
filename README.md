# Valhalla Mobile

[![Valhalla](https://img.shields.io/badge/Valhalla-3.6.3-blue)](https://github.com/valhalla/valhalla/releases/tag/3.6.3)

Fork of [Rallista/valhalla-mobile](https://github.com/Rallista/valhalla-mobile) with support for `optimized_route` (TSP).

This project builds [valhalla](https://github.com/valhalla/valhalla) as a static iOS or shared Android library.

## Integration

### iOS — Swift Package Manager

```
https://github.com/Churikov0112/valhalla-mobile.git
```

Version `0.5.3`, up to next major.

### Android — GitHub Packages

`android/build.gradle.kts`:

```kotlin
allprojects {
    repositories {
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

`android/app/build.gradle.kts`:

```kotlin
implementation("com.github.churikov0112:valhalla-mobile:0.5.3")
```

> Requires a classic GitHub PAT with `read:packages` scope in `~/.gradle/gradle.properties`:
> ```properties
> gpr.key=ghp_...
> gpr.user=Churikov0112
> ```

## Development

See [DEVELOPER.md](DEVELOPER.md) for:
- Project architecture
- How to add a new method (step-by-step)
- How to rebuild the native libraries

## Building from Source

### Prerequisites

```sh
git submodule update --init --recursive
git clone https://github.com/microsoft/vcpkg && git -C vcpkg checkout 2025.12.12
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=`pwd`/vcpkg
```

### iOS

```sh
./build.sh ios clean
```

### Android

Requires NDK `29.0.14206865`. See [docs/development.md](docs/development.md).

```sh
./build.sh android clean
```

## References

- [Valhalla](https://github.com/valhalla/valhalla)
- [Original Rallista/valhalla-mobile](https://github.com/Rallista/valhalla-mobile)
