#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <android/log.h>

#include "logging.h"
#include "hook.h"
#include "android.h"
#include "config.h"
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

//#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "MiPushFake", __VA_ARGS__)

class MiPushFakeModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
		Config::Load();
		LOGD("Config loaded");

    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        // Use JNI to fetch our process name
        const char *process = env->GetStringUTFChars(args->nice_name, nullptr);
		// Use UNI to fetch our user dir
		const char *app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
		
		if (process == nullptr || app_data_dir == nullptr) {
			return;
		}
		if(*process) {
			sscanf(process, "%s", saved_process_name);
		}

		if(*app_data_dir) {
			// /data/user/<user_id>/<package>
			if (sscanf(app_data_dir, "/data/%*[^/]/%d/%s", &saved_uid, saved_package_name) == 2)
				goto found;

			// /mnt/expand/<id>/user/<user_id>/<package>
			if (sscanf(app_data_dir, "/mnt/expand/%*[^/]/%*[^/]/%d/%s", &saved_uid, saved_package_name) == 2)
				goto found;

			// /data/data/<package>
			if (sscanf(app_data_dir, "/data/%*[^/]/%s", saved_package_name) == 1)
				goto found;

			// nothing found
			saved_package_name[0] = '\0';

			found:;
		}
		env->ReleaseStringUTFChars(args->nice_name, process);
		env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);

        preSpecialize(saved_package_name);
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override {
		api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
		LOGD("uid=%d, package=%s, process=%s", saved_uid, saved_package_name, saved_process_name);
        // Inject if module was loaded, otherwise this would've been unloaded by now (for non-GMS)
        if (hook) {
            LOGI("Install hook... for %d:%s", saved_uid / 100000, saved_package_name);
			injectBuild(env);
			Hook::install();
            LOGI("Hook installed");
        }
    }

private:
    Api *api;
    JNIEnv *env;
	bool hook = false;
	char saved_process_name[256] = {0};
	char saved_package_name[256] = {0};
	int saved_uid;


    void preSpecialize(const char *package) {
		if(Config::Packages::Find(package) == false) {
			LOGD("mipushfake: preSpecialize: process=[%s]", package);

        	api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
			hook = false;
			return;
		}
		hook = true;
	 	// Force DenyList unmounting for all GMS processes
        api->setOption(zygisk::FORCE_DENYLIST_UNMOUNT);


    }

	void injectBuild(JNIEnv *env) {
		if (env == nullptr) {
			LOGW("failed to inject android.os.Build for %s due to env is null", saved_package_name);
			return;
		}
		LOGI("inject android.os.Build for %s ", saved_package_name);

		jclass build_class = env->FindClass("android/os/Build");
		if (build_class == nullptr) {
			LOGW("failed to inject android.os.Build for %s due to build is null", saved_package_name);
			return;
		}

		LOGI("inject android.os.Build.VERSION for %s ", saved_package_name);
		jclass version_class = env->FindClass("android/os/Build$VERSION");
		if (version_class == nullptr) {
			LOGW("failed to inject android.os.Build.VERSION for %s due to version is null", saved_package_name);
			return;
		}


		jstring new_str = env->NewStringUTF("Xiaomi");
		jstring new_str2 = env->NewStringUTF("zeus");
		jstring new_str3 = env->NewStringUTF("2201122C");
		jstring new_str4 = env->NewStringUTF("Xiaomi/zeus/zeus:12/SKQ1.211006.001/V13.0.21.0.SLBCNXM:user/release-keys"); 
		jstring new_str5 = env->NewStringUTF("SKQ1.211006.001"); 
		jstring new_str6 = env->NewStringUTF("V13.0.21.0.SLBCNXM"); 
		jstring new_str7 = env->NewStringUTF("SKQ1.211006.001 test-keys"); 

		jfieldID brand_id = env->GetStaticFieldID(build_class, "BRAND", "Ljava/lang/String;");
		if (brand_id != nullptr) {
			env->SetStaticObjectField(build_class, brand_id, new_str);
		}

		jfieldID manufacturer_id = env->GetStaticFieldID(build_class, "MANUFACTURER", "Ljava/lang/String;");
		if (manufacturer_id != nullptr) {
			env->SetStaticObjectField(build_class, manufacturer_id, new_str);
		}

		jfieldID product_id = env->GetStaticFieldID(build_class, "PRODUCT", "Ljava/lang/String;");
		if (product_id != nullptr) {
			env->SetStaticObjectField(build_class, product_id, new_str);
		}

		jfieldID device_id = env->GetStaticFieldID(build_class, "DEVICE", "Ljava/lang/String;");
		if (product_id != nullptr) {
			env->SetStaticObjectField(build_class, device_id, new_str2);
		}

		jfieldID hardware_id = env->GetStaticFieldID(build_class, "HARDWARE", "Ljava/lang/String;");
		if (product_id != nullptr) {
			env->SetStaticObjectField(build_class, hardware_id, new_str2);
		}

		jfieldID model_id = env->GetStaticFieldID(build_class, "MODEL", "Ljava/lang/String;");
		if (product_id != nullptr) {
			env->SetStaticObjectField(build_class, model_id, new_str3);
		}

		jfieldID fingerprint_id = env->GetStaticFieldID(build_class, "FINGERPRINT", "Ljava/lang/String;");
		if (product_id != nullptr) {
			env->SetStaticObjectField(build_class, fingerprint_id, new_str4);
		}

		jfieldID id_id = env->GetStaticFieldID(build_class, "ID", "Ljava/lang/String;");
		if (id_id != nullptr) {
			env->SetStaticObjectField(build_class, id_id, new_str5);
		}

		jfieldID display_id = env->GetStaticFieldID(build_class, "DISPLAY", "Ljava/lang/String;");
		if (display_id != nullptr) {
			env->SetStaticObjectField(build_class, display_id, new_str7);
		}

		jfieldID incremental_id = env->GetStaticFieldID(version_class, "INCREMENTAL", "Ljava/lang/String;");
		if (incremental_id != nullptr) {
			env->SetStaticObjectField(version_class, incremental_id, new_str6);
		}

		if(env->ExceptionCheck())
		{
			env->ExceptionClear();
		}

		env->DeleteLocalRef(new_str);
		env->DeleteLocalRef(new_str2);
		env->DeleteLocalRef(new_str3);
		env->DeleteLocalRef(new_str4);
		env->DeleteLocalRef(new_str5);
		env->DeleteLocalRef(new_str6);
		env->DeleteLocalRef(new_str7);
	}

};

REGISTER_ZYGISK_MODULE(MiPushFakeModule)
