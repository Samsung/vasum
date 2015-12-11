/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki (m.malicki2@samsung.com)
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License
 */

/**
 * @file
 * @author  Mateusz Malicki (m.malicki2@samsung.com)
 * @brief   GVariant visitor
 */

#ifndef CARGO_GVARIANT_INTERNALS_TO_GVARIANT_VISITOR_HPP
#define CARGO_GVARIANT_INTERNALS_TO_GVARIANT_VISITOR_HPP

#include "cargo/internals/is-visitable.hpp"
#include "cargo/internals/is-union.hpp"
#include "cargo/internals/is-like-tuple.hpp"
#include "cargo/types.hpp"
#include "cargo/internals/visit-fields.hpp"

#include <array>
#include <string>
#include <vector>
#include <map>
#include <glib.h>
#include <utility>

namespace cargo {

namespace internals {

class ToGVariantVisitor {

public:
    ToGVariantVisitor()
        : mBuilder(g_variant_builder_new(G_VARIANT_TYPE_TUPLE))
    {
    }

    ToGVariantVisitor(const ToGVariantVisitor& visitor)
        : mBuilder(visitor.mBuilder ? g_variant_builder_ref(visitor.mBuilder) : nullptr)
    {
    }

    ~ToGVariantVisitor()
    {
        if (mBuilder) {
            g_variant_builder_unref(mBuilder);
        }
    }

    ToGVariantVisitor& operator=(const ToGVariantVisitor&) = delete;

    GVariant* toVariant()
    {
        if (mBuilder) {
            GVariant* ret = g_variant_builder_end(mBuilder);
            g_variant_builder_unref(mBuilder);
            mBuilder = nullptr;
            return ret;
        }
        return nullptr;
    }

    template<typename T>
    void visit(const std::string& /* name */, const T& value)
    {
        writeInternal(value);
    }
private:
    GVariantBuilder* mBuilder;

    void writeInternal(std::int8_t value) {
        add("y", value);
    };
    void writeInternal(std::int16_t value) {
        add("n", value);
    };
    void writeInternal(std::int32_t value) {
        add("i", value);
    };
    void writeInternal(std::int64_t value) {
        add("x", value);
    };
    void writeInternal(std::uint8_t value) {
        add("y", value);
    };
    void writeInternal(std::uint16_t value) {
        add("q", value);
    };
    void writeInternal(std::uint32_t value) {
        add("u", value);
    };
    void writeInternal(std::uint64_t value) {
        add("t", value);
    };
    void writeInternal(bool value) {
        add("b", value);
    };
    void writeInternal(double value) {
        add("d", value);
    };
    void writeInternal(const std::string& value) {
        add("s", value.c_str());
    };
    void writeInternal(const char* value) {
        add("s", value);
    };
    void writeInternal(const cargo::FileDescriptor& value) {
        add("h", value.value);
    };

    template<typename T>
    void writeInternal(const std::vector<T>& value)
    {
        if (!value.empty()) {
            g_variant_builder_open(mBuilder, G_VARIANT_TYPE_ARRAY);
            for (const T& v : value) {
                writeInternal(v);
            }
            g_variant_builder_close(mBuilder);
        } else {
            g_variant_builder_add(mBuilder, "as", NULL);
        }
    }

    template<typename T, std::size_t N>
    void writeInternal(const std::array<T, N>& values)
    {
        if (!values.empty()) {
            g_variant_builder_open(mBuilder, G_VARIANT_TYPE_ARRAY);
            for (const T& v : values) {
                writeInternal(v);
            }
            g_variant_builder_close(mBuilder);
        } else {
            g_variant_builder_add(mBuilder, "as", NULL);
        }
    }

    template<typename V>
    void writeInternal(const std::map<std::string, V>& values)
    {
        if (!values.empty()) {
            g_variant_builder_open(mBuilder, G_VARIANT_TYPE_TUPLE);
            for (const auto& v : values) {
                writeInternal(v);
            }
            g_variant_builder_close(mBuilder);
        } else {
            g_variant_builder_add(mBuilder, "as", NULL);
        }
    }

    template<typename T, typename std::enable_if<isLikeTuple<T>::value, int>::type = 0>
    void writeInternal(const T& values)
    {
        g_variant_builder_open(mBuilder, G_VARIANT_TYPE_TUPLE);
        visitFields(values, this, std::string());
        g_variant_builder_close(mBuilder);
    }

    template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
    void writeInternal(const T& value)
    {
        writeInternal(static_cast<const typename std::underlying_type<T>::type>(value));
    }

    template<typename T>
    typename std::enable_if<isVisitable<T>::value && !isUnion<T>::value>::type
    writeInternal(const T& value)
    {
        ToGVariantVisitor visitor;
        value.accept(visitor);
        g_variant_builder_add_value(mBuilder, visitor.toVariant());
    }

    template<typename T>
    typename std::enable_if<isVisitable<T>::value && isUnion<T>::value>::type
    writeInternal(const T& value)
    {
        ToGVariantVisitor visitor;
        value.accept(visitor);
        add("v", visitor.toVariant());
    }

    template<typename Value>
    void add(const char* type, Value value) {
        g_variant_builder_add(mBuilder, type, value);
    }
};

} // namespace internals

} // namespace cargo

#endif // CARGO_GVARIANT_INTERNALS_TO_GVARIANT_VISITOR_HPP
