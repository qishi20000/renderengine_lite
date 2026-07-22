plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.rel.avmdemo"
    compileSdk = 34
    // Pinned explicitly (rather than letting AGP pick "whatever's newest
    // installed") so `externalNativeBuild` CMake invocations are
    // reproducible across machines/CI. Must be installed via
    // `sdkmanager --install "ndk;27.1.12297006"` (or Android Studio's SDK
    // Manager) — this is also the exact NDK used to validate the native
    // build end-to-end (see README.md "当前状态").
    ndkVersion = "27.1.12297006"

    defaultConfig {
        applicationId = "com.rel.avmdemo"
        minSdk = 26 // AHardwareBuffer + EGL_ANDROID_get_native_client_buffer baseline (see ARCHITECTURE.md sec.7)
        targetSdk = 34
        versionCode = 1
        versionName = "0.1.0-scaffold"

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++17"
                arguments += listOf("-DANDROID_STL=c++_shared")
            }
        }
        ndk {
            // Adreno targets: arm64-v8a covers all shipping Snapdragon
            // automotive SoCs relevant here; armeabi-v7a dropped to keep
            // the APK lightweight per the project's "轻量化" goal.
            abiFilters += listOf("arm64-v8a")
        }
    }

    externalNativeBuild {
        cmake {
            // Points at the JNI-bridge CMakeLists.txt, which in turn
            // add_subdirectory()'s the repo root CMakeLists.txt to build
            // renderengine_lite itself. See src/main/cpp/CMakeLists.txt.
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
}
