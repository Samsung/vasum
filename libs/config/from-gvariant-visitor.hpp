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

#ifndef COMMON_CONFIG_FROM_GVARIANT_VISITOR_HPP
#define COMMON_CONFIG_FROM_GVARIANT_VISITOR_HPP

#include "config/is-visitable.hpp"
#include "config/exception.hpp"
#include "config/is-union.hpp"
#include "config/types.hpp"
#include "config/visit-fields.hpp"

#include <string>
#include <vector>
#include <array>
#include <utility>
#include <memory>
#include <cassert>

#include <glib.h>

namespace config {

class FromGVariantVisitor {
public:
    explicit FromGVariantVisitor(GVariant* variant)
    {
        //Assume that the visited object is not a union
        checkType(variant, G_VARIANT_TYPE_TUPLE);
        mIter = g_variant_iter_new(variant);
    }

    FromGVariantVisitor(const FromGVariantVisitor& visitor)
        : mIter(g_variant_iter_copy(visitor.mIter))
    {
    }

    ~FromGVariantVisitor()
    {
        g_variant_iter_free(mIter);
    }

    FromGVariantVisitor& operator=(const FromGVariantVisitor&) = delete;

    template<typename T>
    void visit(const std::string& name, T& value)
    {
        auto child = makeUnique(g_variant_iter_next_value(mIter));
        if (!child) {
            throw config::ConfigException(
                "GVariant doesn't match with config. Can't set  '" + name + "'");
        }
        fromGVariant(child.get(), value);
    }

private:
    GVariantIter* mIter;

    static std::unique_ptr<GVariant, decltype(&g_variant_unref)> makeUnique(GVariant* variant)
    {
        return std::unique_ptr<GVariant, decltype(&g_variant_unref)>(variant, g_variant_unref);
    }

    static void checkType(GVariant* object, const GVariantType* type)
    {
        if (!g_variant_is_of_type(object, type)) {
            throw ConfigException("Invalid field type");
        }
    }

    static void fromGVariant(GVariant* object, std::int32_t& value)
    {
        checkType(object, G_VARIANT_TYPE_INT32);
        value = g_variant_get_int32(object);
    }

    static void fromGVariant(GVariant* object, std::int64_t& value)
    {
        checkType(object, G_VARIANT_TYPE_INT64);
        value = g_variant_get_int64(object);
    }

    static void fromGVariant(GVariant* object, std::uint8_t& value)
    {
        checkType(object, G_VARIANT_TYPE_BYTE);
        value = g_variant_get_byte(object);
    }

    static void fromGVariant(GVariant* object, std::uint32_t& value)
    {
        checkType(object, G_VARIANT_TYPE_UINT32);
        value = g_variant_get_uint32(object);
    }

    static void fromGVariant(GVariant* object, std::uint64_t& value)
    {
        checkType(object, G_VARIANT_TYPE_UINT64);
        value = g_variant_get_uint64(object);
    }

    static void fromGVariant(GVariant* object, bool& value)
    {
        checkType(object, G_VARIANT_TYPE_BOOLEAN);
        value = g_variant_get_boolean(object);
    }

    static void fromGVariant(GVariant* object, double& value)
    {
        checkType(object, G_VARIANT_TYPE_DOUBLE);
        value = g_variant_get_double(object);
    }

    static void fromGVariant(GVariant* object, std::string& value)
    {
        checkType(object, G_VARIANT_TYPE_STRING);
        value = g_variant_get_string(object, NULL);
    }

    static void fromGVariant(GVariant* object, char* &value)
    {
        checkType(object, G_VARIANT_TYPE_STRING);

        const char* source = g_variant_get_string(object, NULL);
        size_t len = std::strlen(source);

        value = new char[len + 1];
        std::strncpy(value, source, len);
        value[len] = '\0';
    }

    static void fromGVariant(GVariant* object, config::FileDescriptor& value)
    {
        checkType(object, G_VARIANT_TYPE_INT32);
        value.value = g_variant_get_int32(object);
    }

    template<typename T>
    static void fromGVariant(GVariant* object, std::vector<T>& value)
    {
        checkType(object, G_VARIANT_TYPE_ARRAY);
        GVariantIter iter;
        g_variant_iter_init(&iter, object);
        int length = g_variant_iter_n_children(&iter);
        value.resize(static_cast<size_t>(length));
        for (int i = 0; i < length; ++i) {
            auto child = makeUnique(g_variant_iter_next_value(&iter));
            assert(child);
            fromGVariant(child.get(), value[static_cast<size_t>(i)]);
        }
    }

    template<typename T, std::size_t N>
    static void fromGVariant(GVariant* object, std::array<T, N>& values)
    {
        checkType(object, G_VARIANT_TYPE_ARRAY);

        GVariantIter iter;
        g_variant_iter_init(&iter, object);

        for (T& value: values) {
            auto child = makeUnique(g_variant_iter_next_value(&iter));
            fromGVariant(child.get(), value);
        }
    }

    struct HelperVisitor
    {
        template<typename T>
        static void visit(GVariantIter* iter, T&& value)
        {
            auto child = makeUnique(g_variant_iter_next_value(iter));
            fromGVariant(child.get(), value);
        }
    };

    template<typename ... T>
    static void fromGVariant(GVariant* object, std::pair<T...>& values)
    {
        checkType(object, G_VARIANT_TYPE_ARRAY);

        GVariantIter iter;
        g_variant_iter_init(&iter, object);

        HelperVisitor visitor;
        visitFields(values, &visitor, &iter);
    }

    template<typename T, typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
    static void fromGVariant(GVariant* object, T& value)
    {
        fromGVariant(object,
                     *reinterpret_cast<typename std::underlying_type<T>::type*>(&value));
    }

    template<typename T>
    static typename std::enable_if<isUnion<T>::value>::type
    fromGVariant(GVariant* object, T& value)
    {
        checkType(object, G_VARIANT_TYPE_VARIANT);
        auto inner = makeUnique(g_variant_get_variant(object));

        FromGVariantVisitor visitor(inner.get());
        value.accept(visitor);
    }

    template<typename T>
    static typename std::enable_if<isVisitable<T>::value && !isUnion<T>::value>::type
    fromGVariant(GVariant* object, T& value)
    {
        FromGVariantVisitor visitor(object);
        value.accept(visitor);
    }

};

} // namespace config

#endif // COMMON_CONFIG_FROM_GVARIANT_VISITOR_HPP
