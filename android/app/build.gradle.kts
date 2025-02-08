import java.io.ByteArrayOutputStream
import java.util.Properties

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
}

fun getGitCommitHash(): String {
    return try {
        val stdout = ByteArrayOutputStream()
        exec {
            commandLine("git", "rev-parse", "--short", "HEAD")
            standardOutput = stdout
        }
        stdout.toString().trim()
    } catch (e: Exception) {
        "unknown"
    }
}

android {
    namespace = "org.toni.customfetch_android"
    compileSdk = 35

    defaultConfig {
        applicationId = "org.toni.customfetch_android"
        minSdk = 26
        targetSdk = 35
        versionCode = 1
        versionName = "0.10.2"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            cmake {
                cppFlags += "-I${rootDir}/../include -DANDROID_APP=1"
                targets("customfetch")
            }
        }
    }

    signingConfigs {
        create("release") {
            val propertiesFile = rootProject.file("signing.properties")
            if (propertiesFile.exists()) {
                val properties = Properties().apply {
                    load(propertiesFile.reader())
                }
                storeFile = File(properties.getProperty("storeFilePath"))
                storePassword = properties.getProperty("storePassword")
                keyPassword = properties.getProperty("keyPassword")
                keyAlias = properties.getProperty("keyAlias")
            }
        }
    }

    buildTypes {
        getByName("debug") {
            buildConfigField("String", "GIT_COMMIT_HASH", "\"${getGitCommitHash()}\"")
        }
        getByName("release") {
            buildConfigField("String", "GIT_COMMIT_HASH", "\"${getGitCommitHash()}\"")
        }
        release {
            getByName("release") {
                signingConfig = signingConfigs.getByName("release")
            }
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }
    buildFeatures {
        viewBinding = true
        buildConfig = true
    }
    externalNativeBuild {
        cmake {
            path = file("../CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

dependencies {
    implementation(libs.colorpickerview)
    implementation(libs.colorpickerpref)

    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    implementation(libs.androidx.constraintlayout)
    implementation(libs.androidx.navigation.fragment.ktx)
    implementation(libs.androidx.navigation.ui.ktx)
    implementation(libs.androidx.preference.ktx)
    implementation(libs.androidx.preference)
}
