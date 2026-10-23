# Third-Party Notices

Northrend uses the third-party software listed below. These components retain
their own copyright and license terms; Northrend's MIT OR Apache-2.0 license
does not replace them. Complete license texts are stored in
[licenses/third-party](licenses/third-party).

Distributions containing Northrend should include this notice and the
applicable license texts.

## SDL3

- Version: 3.4.16
- Source: <https://github.com/libsdl-org/SDL>
- License: zlib
- License text: [SDL3.txt](licenses/third-party/SDL3.txt)

Northrend also contains SDL's Android Java integration. SDL's bundled HIDAPI
code is used under its permissive BSD license option, and its yuv2rgb code is
licensed under the BSD 3-Clause license.

- HIDAPI: [SDL3-HIDAPI-BSD.txt](licenses/third-party/SDL3-HIDAPI-BSD.txt)
- yuv2rgb: [SDL3-yuv2rgb-BSD.txt](licenses/third-party/SDL3-yuv2rgb-BSD.txt)
- Wayland protocol definitions used by SDL's Linux backend:
  [Wayland-Protocols.txt](licenses/third-party/Wayland-Protocols.txt)

## Flecs

- Version: 4.1.6
- Source: <https://github.com/SanderMertens/flecs>
- License: MIT
- License text: [Flecs.txt](licenses/third-party/Flecs.txt)

## GLM

- Version: 1.0.3
- Source: <https://github.com/g-truc/glm>
- License: MIT OR Happy Bunny License
- License text: [GLM.txt](licenses/third-party/GLM.txt)

## volk

- Revision: 54fc0d7a72887cbfc5b030d26897b2b40ac53fb8
- Source: <https://github.com/zeux/volk>
- License: MIT
- License text: [volk.txt](licenses/third-party/volk.txt)

## Vulkan-Headers

- Revision: 6802bb4733b63ed5efd3adb308a6c885ef180ea1
- Source: <https://github.com/KhronosGroup/Vulkan-Headers>
- License: Apache-2.0 OR MIT, depending on the individual file
- License information:
  [Vulkan-Headers.txt](licenses/third-party/Vulkan-Headers.txt)

The complete Apache-2.0 and MIT license texts are also available in
[LICENSE-APACHE](LICENSE-APACHE) and [LICENSE-MIT](LICENSE-MIT).

## Gradle Wrapper

- Source: <https://github.com/gradle/gradle>
- License: Apache-2.0
- License text: [Gradle.txt](licenses/third-party/Gradle.txt)

The Gradle Wrapper is build tooling and is not linked into Northrend or a
generated game binary.

## Eclipse Temurin

Android builds use a Java Development Kit to run Gradle. When no suitable
local JDK is available, Northrend downloads Eclipse Temurin 17 into the
consumer's build directory.

- Version: 17.0.20.1+1
- Source: <https://github.com/adoptium/temurin17-binaries>
- License information: included in the downloaded JDK archive

The downloaded JDK is build tooling and is not linked into Northrend or a
generated game binary.

## Android SDK Command-Line Tools

When no suitable Android SDK is available, Android builds download Google's
command-line tools and use them to install the pinned SDK packages in the
consumer's build directory.

- Revision: 15859902
- Source: <https://developer.android.com/studio#command-line-tools-only>
- License information: included in the downloaded SDK components

The Android SDK is build tooling and is not linked into Northrend or a
generated game binary. Installing SDK packages requires acceptance of
Google's Android SDK license terms.

## Android NDK Toolchain

Android builds currently link the NDK's static C++ runtime into the generated
native library. The default NDK version is 28.2.13676358. Its LLVM and libc++
components retain their upstream license terms.

- Complete toolchain notice:
  [Android-NDK-toolchain-NOTICE.txt](licenses/third-party/Android-NDK-toolchain-NOTICE.txt)

Games that distribute an Android package built with a different NDK version
should include the notice supplied with that exact NDK version.
