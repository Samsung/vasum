cargo libraries usage example
=============================

@ingroup libcargo

@code
#include "cargo/fields.hpp"
#include "cargo-gvariant/cargo-gvariant.hpp"
#include "cargo-json/cargo-json.hpp"
#include "cargo-sqlite/cargo-sqlite.hpp"
#include "cargo-sqlite-json/cargo-sqlite-json.hpp"
#include "cargo-fd/cargo-fd.hpp"
#include <iostream>
#include <cstdio>

struct Foo
{
    std::string bar = "plain-text";
    std::vector<int> tab = std::vector<int>{1, 2, 4, 8};
    double number = 3.14;

    CARGO_REGISTER
    (
        bar,
        tab,
        number
    )
};

int main()
{
    Foo foo;

    const std::string jsonString = cargo::saveToJsonString(foo);
    cargo::loadFromJsonString(jsonString, foo);

    const GVariant* gVariantPointer = cargo::saveToGVariant(foo);
    cargo::loadFromGVariant(gVariantPointer, foo);
    g_variant_unref(gVariantPointer);

    constexpr std::string jsonFile = "foo.json";
    cargo::saveToJsonFile(jsonFile, foo);
    cargo::loadFromJsonFile(jsonFile, foo);

    constexpr std::string kvDBPath = "kvdb";
    constexpr std::string key = "foo";
    cargo::saveToKVStore(kvDBPath, foo, key);
    cargo::loadFromKVStore(kvDBPath, foo, key);

    cargo::loadFromKVStoreWithJson(kvDBPath, jsonString, foo, key);
    cargo::loadFromKVStoreWithJsonFile(kvDBPath, jsonFile, foo, key);

    FILE* file = fopen("blob", "wb");
    if (!file)
    {
        return EXIT_FAILURE;
    }
    const int fd = ::fileno(file);
    cargo::saveToFD(fd, foo);
    ::fclose(file);
    file = ::fopen("blob", "rb");
    if(!file) {
        return EXIT_FAILURE;
    }
    cargo::loadFromFD(fd, foo);
    ::fclose(file);

    return 0;
}
@endcode
