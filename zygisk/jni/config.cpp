#include <map>
#include <vector>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/system_properties.h>
#include "config.h"
#include "logging.h"

using namespace Config;

namespace Config {

    namespace Properties {

        void Put(const char *name, const char *value);
    }

    namespace Packages {

        void Add(const char *name);
    }
}


static std::map<std::string, Property *> props;
static std::vector<std::string> packages;

Property *Properties::Find(const char *name) {
    if (!name) return nullptr;

    auto it = props.find(name);
    if (it != props.end()) {
        return it->second;
    }
    return nullptr;
}

void Properties::Put(const char *name, const char *value) {
    if (!name) return;

    auto prop = Find(name);
    delete prop;

    props[name] = new Property(name, value ? value : "");

    LOGD("property: %s %s", name, value);
}

bool Packages::Find(const char *name) {
    if (!name) return false;
    return std::find(packages.begin(), packages.end(), name) != packages.end();
}

void Packages::Add(const char *name) {
    if (!name) return;
    packages.emplace_back(name);

    LOGD("package: %s", name);
}

void Config::Load() {
	Properties::Put("ro.product.manufacturer", "Xiaomi");
	Properties::Put("ro.product.brand", "Xiaomi");
	Properties::Put("ro.product.name", "Xiaomi");
	Properties::Put("ro.product.device", "zeus");
	Properties::Put("ro.hardware", "zeus");
	Properties::Put("ro.product.model", "2201122C");
	Properties::Put("ro.build.fingerprint", "Xiaomi/zeus/zeus:12/SKQ1.211006.001/V13.0.21.0.SLBCNXM:user/release-keys");

	Properties::Put("ro.build.id", "SKQ1.211006.001");
	Properties::Put("ro.build.version.incremental", "V13.0.21.0.SLBCNXM");
	Properties::Put("ro.build.display.id", "SKQ1.211006.001 test-keys");

	Packages::Add("com.taobao.taobao");
	Packages::Add("com.tencent.weread");
	Packages::Add("com.taobao.idlefish");
	Packages::Add("cmb.pb");
	Packages::Add("cn.adidas.app");
	Packages::Add("cn.adidas.confirmed.app");
	Packages::Add("com.autonavi.minimap");
	Packages::Add("com.coolapk.market");
	Packages::Add("com.dianping.v1");
	Packages::Add("com.icbc");
	Packages::Add("com.sankuai.meituan");
	Packages::Add("com.smzdm.client.android");
	Packages::Add("com.starbucks.cn");
	Packages::Add("me.ele");
	Packages::Add("com.wudaokou.hippo");
	Packages::Add("com.sf.activity");
	Packages::Add("cn.com.hkgt.gasapp");
	Packages::Add("com.heytap.health");
	Packages::Add("com.alicloud.databox");
}

