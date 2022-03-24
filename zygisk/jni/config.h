#pragma once

#include <string>
#include <vector>

namespace Config {

    struct Property {

        std::string name;
        std::string value;

        Property(const char *name, const char *value) : name(name), value(value) {}
    };

    void Load(std::vector<char> config);

    namespace Properties {

        Property *Find(const char *name);
    }

    namespace Packages {

        bool Find(const char *name);
    }
}
