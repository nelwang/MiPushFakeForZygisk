#include <cstdlib>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <android/log.h>

#include "logging.h"
#include "hook.h"
#include "android.h"
#include "config.h"
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

static constexpr auto CONFIG_PATH = "/data/adb/modules/mipushfake/config";

class MiPushFakeModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
		loadPayload();
		Config::Load(config);
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

	std::vector<char> config;
	bool hook = false;
	char saved_process_name[256] = {0};
	char saved_package_name[256] = {0};
	int saved_uid;

    static int receiveFile(int remote_fd, std::vector<char>& buf) {
        off_t size;
        int ret = read(remote_fd, &size, sizeof(size));
        if (ret < 0) {
            LOGE("Failed to read size");
            return -1;
        }

        buf.resize(size);

        int bytesReceived = 0;
        while (bytesReceived < size) {
            ret = read(remote_fd, buf.data() + bytesReceived, size - bytesReceived);
            if (ret < 0) {
                LOGE("Failed to read data");
                return -1;
            }

            bytesReceived += ret;
        }

        return bytesReceived;
    }

    void loadPayload() {
        auto fd = api->connectCompanion();

        auto size = receiveFile(fd, config);
        LOGD("Loaded module payload: %d bytes", size);

        close(fd);
    }

    void preSpecialize(const char *package) {
		if(Config::Packages::Find(package) == false) {
			LOGD("mipushfake: preSpecialize: package=[%s]", package);

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

		jstring new_str = env->NewStringUTF("Xiaomi");

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


		if(env->ExceptionCheck())
		{
			env->ExceptionClear();
		}

		env->DeleteLocalRef(new_str);
	}



};

static off_t sendFile(int remote_fd, const std::string& path) {
    auto in_fd = open(path.c_str(), O_RDONLY);
    if (in_fd < 0) {
        LOGE("Failed to open file %s: %d (%s)", path.c_str(), errno, strerror(errno));
        return -1;
    }

    auto size = lseek(in_fd, 0, SEEK_END);
    if (size < 0) {
        LOGI("Failed to get file size");
        close(in_fd);
        return -1;
    }
    lseek(in_fd, 0, SEEK_SET);

    // Send size first for buffer allocation
    int ret = write(remote_fd, &size, sizeof(size));
    if (ret < 0) {
        LOGI("Failed to send size");
        close(in_fd);
        return -1;
    }

    ret = sendfile(remote_fd, in_fd, nullptr, size);
    if (ret < 0) {
        LOGI("Failed to send data");
        close(in_fd);
        return -1;
    }

    close(in_fd);
    return size;
}

static void companionHandler(int remote_fd) {
    // Serve module dex
    auto size = sendFile(remote_fd, CONFIG_PATH);
    LOGD("Sent module payload: %ld bytes", size);
}

REGISTER_ZYGISK_COMPANION(companionHandler)
REGISTER_ZYGISK_MODULE(MiPushFakeModule)
