#include <map>
#include <vector>
#include <string>
#include <sstream>
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

void Config::Load(std::vector<char> config) {
	std::string s_config(config.begin(), config.end());
	LOGD("Config::Load: %s", s_config.c_str());
	std::string app;
	std::stringstream content(s_config.c_str());
	while(std::getline(content, app, '\n')) {
		if(app.length() == 0) {
			continue;
		}
		LOGD("Config::Load: %s", app.c_str());
		Packages::Add(app.c_str());
	}
	LOGD("Config::Load app list loaded.");

	Properties::Put("ro.product.manufacturer", "Xiaomi");
	Properties::Put("ro.product.brand", "Xiaomi");
	Properties::Put("ro.product.name", "Xiaomi");

}

