/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @brief   Macros for declare cargo fields
 */

#ifndef CARGO_FIELDS_UNION_HPP
#define CARGO_FIELDS_UNION_HPP

#include "cargo/fields.hpp"
#include "cargo/exception.hpp"

#include <utility>
#include <boost/any.hpp>

class DisableMoveAnyWrapper : public boost::any
{
    public:
        DisableMoveAnyWrapper() {}
        DisableMoveAnyWrapper(const DisableMoveAnyWrapper& any)
            : boost::any(static_cast<const boost::any&>(any)) {};
        DisableMoveAnyWrapper& operator=(DisableMoveAnyWrapper&& any) = delete;
        DisableMoveAnyWrapper& operator=(const DisableMoveAnyWrapper& any) {
            static_cast<boost::any&>(*this) = static_cast<const boost::any&>(any);
            return *this;
        }
};

/**
 * @ingroup libCargo
 *
 * Use this macro to declare and register cargo fields.
 *
 * Example of fields registration:
 * @code
 *  struct Foo {
 *      std::string bar;
 *
 *      CARGO_REGISTER
 *      (
 *          bar,
 *      )
 *  };
 *
 *  struct Config
 *  {
 *      CARGO_DECLARE_UNION
 *      (
 *          Foo,
 *          int
 *      )
 *  };
 * @endcode
 *
 * Example of valid configuration:
 * @code
 *   1. {
 *        "type": "Foo",
 *        "value": { "bar": "some string" }
 *      }
 *   2. {
 *        "type": "int",
 *        "value": 1
 *      }
 * @endcode
 *
 * Usage of existing bindings:
 * @code
 *   Config config;
 *   // ...
 *   if (config.isSet()) {
 *      if (config.is<Foo>()) {
 *          Foo& foo = config.as<Foo>();
 *          // ...
 *      }
 *      if (config.is<int>())) {
 *          int field = config.as<int>();
 *          // ...
 *      }
 *   } else {
 *     // Config is not set
 *     Foo foo({"some string"});
 *     config.set(foo);
 *     config.set(std::move(foo));         //< copy sic!
 *     config.set(Foo({"some string"}));
 *   }
 * @endcode
 */
#define CARGO_DECLARE_UNION(...)                                                                \
private:                                                                                        \
    DisableMoveAnyWrapper mCargoDeclareField;                                                   \
                                                                                                \
    template<typename Visitor>                                                                  \
    void visitOption(Visitor& v, const std::string& name) {                                     \
        GENERATE_CODE(GENERATE_UNION_VISIT__, __VA_ARGS__)                                      \
        throw cargo::CargoException("Union type error. Unsupported type");                      \
    }                                                                                           \
    template<typename Visitor>                                                                  \
    void visitOption(Visitor& v, const std::string& name) const {                               \
        GENERATE_CODE(GENERATE_UNION_VISIT_CONST__, __VA_ARGS__)                                \
        throw cargo::CargoException("Union type error. Unsupported type");                      \
    }                                                                                           \
    std::string getOptionName() const {                                                         \
        GENERATE_CODE(GENERATE_UNION_NAME__, __VA_ARGS__)                                       \
        return std::string();                                                                   \
    }                                                                                           \
    boost::any& getHolder() {                                                                   \
        return static_cast<boost::any&>(mCargoDeclareField);                                    \
    }                                                                                           \
    const boost::any& getHolder() const {                                                       \
        return static_cast<const boost::any&>(mCargoDeclareField);                              \
    }                                                                                           \
public:                                                                                         \
                                                                                                \
    template<typename Visitor>                                                                  \
    void accept(Visitor v) {                                                                    \
        std::string name;                                                                       \
        v.visit("type", name);                                                                  \
        visitOption(v, name);                                                                   \
    }                                                                                           \
                                                                                                \
    template<typename Visitor>                                                                  \
    void accept(Visitor v) const {                                                              \
        const std::string name = getOptionName();                                               \
        if (name.empty()) {                                                                     \
           throw cargo::CargoException("Type is not set");                                      \
        }                                                                                       \
        v.visit("type", name);                                                                  \
        visitOption(v, name);                                                                   \
    }                                                                                           \
                                                                                                \
    template<typename Type>                                                                     \
    bool is() const {                                                                           \
        return boost::any_cast<Type>(&getHolder()) != NULL;                                     \
    }                                                                                           \
    template<typename Type>                                                                     \
    typename std::enable_if<!std::is_const<Type>::value, Type>::type& as() {                    \
        if (getHolder().empty()) {                                                              \
            throw cargo::CargoException("Type is not set");                                     \
        }                                                                                       \
        return boost::any_cast<Type&>(getHolder());                                             \
    }                                                                                           \
    template<typename Type>                                                                     \
    const Type& as() const {                                                                    \
        if (getHolder().empty()) {                                                              \
            throw cargo::CargoException("Type is not set");                                     \
        }                                                                                       \
        return boost::any_cast<const Type&>(getHolder());                                       \
    }                                                                                           \
    bool isSet() {                                                                              \
        return !getOptionName().empty();                                                        \
    }                                                                                           \
    template<typename Type>                                                                     \
    Type& set(const Type& src) {                                                                \
        getHolder() = std::forward<const Type>(src);                                            \
        if (getOptionName().empty()) {                                                          \
           throw cargo::CargoException("Unsupported type");                                     \
        }                                                                                       \
        return as<Type>();                                                                      \
    }                                                                                           \

#define GENERATE_CODE(MACRO, ...)                                                               \
    BOOST_PP_LIST_FOR_EACH(MACRO, _, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__))

#define GENERATE_UNION_VISIT__(r, _, TYPE_)                                                     \
    if (#TYPE_ == name) {                                                                       \
        v.visit("value", set(std::move(TYPE_())));                                              \
        return;                                                                                 \
    }

#define GENERATE_UNION_VISIT_CONST__(r, _, TYPE_)                                               \
    if (#TYPE_ == name) {                                                                       \
        v.visit("value", as<const TYPE_>());                                                    \
        return;                                                                                 \
    }

#define GENERATE_UNION_NAME__(r, _, TYPE_)                                                      \
    if (is<TYPE_>()) {                                                                          \
        return #TYPE_;                                                                          \
    }

#endif // CARGO_FIELDS_UNION_HPP
