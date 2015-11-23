/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
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
 * @author     Piotr Bartosiewicz (p.bartosiewi@partner.samsung.com)
 * @addtogroup libcargo libcargo
 * @brief      C++ library for handling serialization
 */

#ifndef CARGO_FIELDS_HPP
#define CARGO_FIELDS_HPP

#include <boost/preprocessor/variadic/to_list.hpp>
#include <boost/preprocessor/list/for_each.hpp>

#include "cargo/types.hpp"

#if BOOST_PP_VARIADICS != 1
#error variadic macros not supported
#endif

/**
 * @ingroup libcargo
 * Register empty cargo class
 */
#define CARGO_REGISTER_EMPTY                                       \
    template<typename Visitor>                                     \
    static void accept(Visitor ) {                                 \
    }                                                              \

/**
 * @ingroup libcargo
 *
 * Registers cargo fields within class.
 *
 * Example:
 * @code
 *   struct Foo
 *   {
 *       std::string bar;
 *       std::vector<int> tab;
 *
 *       // SubConfigA must also register config fields
 *       SubConfigA sub_a;
 *
 *       // SubConfigB must also register config fields
 *       std::vector<SubConfigB> tab_sub;
 *
 *       CARGO_REGISTER
 *       (
 *           bar,
 *           tab,
 *           sub_a
 *       )
 *   };
 * @endcode
 */
#define CARGO_REGISTER(...)                                        \
    template<typename Visitor>                                     \
    void accept(Visitor v) {                                       \
        GENERATE_ELEMENTS__(__VA_ARGS__)                           \
    }                                                              \
    template<typename Visitor>                                     \
    void accept(Visitor v) const {                                 \
        GENERATE_ELEMENTS__(__VA_ARGS__)                           \
    }                                                              \

/**
 * @ingroup libcargo
 *
 * Registers new cargo fields within child class.
 *
 * CARGO_REGISTER doesn't preserve cargo fields registered
 * in the base class. Use this macro instead.
 *
 * Example:
 * @code
 *   struct Foo
 *   {
 *       std::string bar;
 *       std::vector<int> tab;
 *
 *       // SubConfigA must also register config fields
 *       SubConfigA sub_a;
 *
 *       // SubConfigB must also register config fields
 *       std::vector<SubConfigB> tab_sub;
 *
 *       CARGO_REGISTER
 *       (
 *           bar,
 *           tab,
 *           sub_a
 *       )
 *   };
 *
 *   struct FooChild : Foo
 *   {
 *       std::string newField;
 *
 *       CARGO_EXTEND(Foo)
 *       (
 *           newField
 *       )
 *   };
 *
 * @endcode
 */
#define CARGO_EXTEND(BASE)                                         \
    typedef BASE ParentVisitor;                                    \
    __CARGO_EXTEND                                                 \

#define __CARGO_EXTEND(...)                                        \
    template<typename Visitor>                                     \
    void accept(Visitor v) {                                       \
        GENERATE_ELEMENTS__(__VA_ARGS__)                           \
        ParentVisitor::accept(v);                                  \
    }                                                              \
    template<typename Visitor>                                     \
    void accept(Visitor v) const {                                 \
        GENERATE_ELEMENTS__(__VA_ARGS__)                           \
        ParentVisitor::accept(v);                                  \
    }                                                              \

#define GENERATE_ELEMENTS__(...)                                   \
    BOOST_PP_LIST_FOR_EACH(GENERATE_ELEMENT__,                     \
                           _,                                      \
                           BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__)) \

#define GENERATE_ELEMENT__(r, _, element)                          \
    v.visit(#element, element);                                    \

#endif // CARGO_FIELDS_HPP
