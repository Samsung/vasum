/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Maciej Karpiuk (m.karpiuk2@samsung.com)
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
 * @author  Maciej Karpiuk (m.karpiuk2@samsung.com)
 * @defgroup validator Validator
 * @ingroup libCargo
 * @brief   Cargo validation functions
 */
#ifndef CARGO_VALIDATOR_VALIDATOR_HPP
#define CARGO_VALIDATOR_VALIDATOR_HPP

#include "cargo/fields.hpp"
#include "cargo-validator/exception.hpp"
#include "cargo-validator/internals/validator-visitor.hpp"
#include "cargo-validator/internals/is-validable.hpp"

namespace cargo {

/*@{*/


#define CARGO_VALIDATE(...)                                     \
    void accept(cargo::validator::ValidatorVisitor v) const {   \
        __VA_ARGS__                                             \
    }                                                           \

#define CARGO_CHECK(func, ...)                                  \
    GENERATE_ELEMENTS_FOR_FUNC_(func, __VA_ARGS__)              \

#define GENERATE_ELEMENTS_FOR_FUNC_(func, ...)                    \
   BOOST_PP_LIST_FOR_EACH(GENERATE_ELEMENT_FOR_FUNC_,             \
                          func,                                   \
                          BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__)) \

#define GENERATE_ELEMENT_FOR_FUNC_(r, func, element)              \
    v.visit(func, #element, element);                             \

#define CARGO_COMPARE(func, arg_A, arg_B)                         \
    v.visit(func, #arg_A, arg_A, #arg_B, arg_B);                  \

namespace validator
{
/**
 * pre-defined checks
 */
bool isNonEmptyString(const std::string &);
// add more checks here..



/**
 * Validate given structure if fields match the requirements.
 *
 * @param visitable     visitable structure to check
 */
template <class Cargo>
void validate(const Cargo& visitable)
{
    static_assert(isValidable<Cargo>::value, "Use CARGO_VALIDATE macro");

    ValidatorVisitor visitor;
    visitable.accept(visitor);
}

} // namespace validator
} // namespace cargo

/*@}*/

#endif // CARGO_VALIDATOR_VALIDATOR_HPP
