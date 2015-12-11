/*
*  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
*
*  Contact: Jan Olszak (j.olszak@samsung.com)
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
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Interface for result builders

 */

#ifndef COMMON_RESULT_METHOD_RESULT_BUILDER_HPP
#define COMMON_RESULT_METHOD_RESULT_BUILDER_HPP

#include <memory>

#include "cargo/internals/is-union.hpp"

namespace vasum {
namespace api {

/**
 * An interface used to set a result to a method call.
 */
class MethodResultBuilder {
public:
    typedef std::shared_ptr<MethodResultBuilder> Pointer;

    virtual ~MethodResultBuilder() {}
    virtual void setVoid() = 0;
    virtual void setError(const std::string& name, const std::string& message) = 0;
    virtual std::string getID() const = 0;

    template<typename Data>
    void set(const std::shared_ptr<Data>& data)
    {
        static_assert(cargo::internals::isVisitable<Data>::value, "Use only Cargo's structures");
        setImpl(data);
    }

private:
    virtual void setImpl(const std::shared_ptr<void>& data) = 0;

};

} // namespace api
} // namespace vasum

#endif // COMMON_RESULT_METHOD_RESULT_BUILDER_HPP
