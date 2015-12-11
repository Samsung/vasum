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
 * @brief   validator visitor
 */

#ifndef CARGO_VALIDATOR_INTERNALS_VALIDATOR_VISITOR_HPP
#define CARGO_VALIDATOR_INTERNALS_VALIDATOR_VISITOR_HPP

#include "cargo-validator/exception.hpp"
#include "cargo/internals/visit-fields.hpp"

#include <string>
#include <functional>

namespace cargo {
namespace validator {

class ValidatorVisitor {
public:
    ValidatorVisitor& operator=(const ValidatorVisitor&) = delete;

    template<typename T, typename Fn>
    void visit(const Fn &func,
               const std::string &field_name,
               const T& value)
    {
        if (!func(value)) {
            const std::string msg = "validation failed on field: " +
                                    field_name +
                                    "(" + std::string(typeid(value).name()) + ")";
            throw VerificationException(msg);
        }
    }

    template<typename A, typename B, typename Fn>
    void visit(const Fn &func,
               const std::string &field_A_name,
               const A& arg_A,
               const std::string &field_B_name,
               const B& arg_B)
    {
        if (!func(arg_A, arg_B)) {
            const std::string msg = "validation failed: improper fields " +
                                    field_A_name + " and " + field_B_name +
                                    " relationship.";
            throw VerificationException(msg);
        }
    }
};

} // namespace validator
} // namespace cargo

#endif // CARGO_VALIDATOR_INTERNALS_VALIDATOR_VISITOR_HPP
