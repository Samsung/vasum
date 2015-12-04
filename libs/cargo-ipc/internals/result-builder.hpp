/*
*  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
*
*  Contact: Jan Olszak <j.olszak@samsung.com>
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
 * @brief   Class for storing result of a method - data or exception
 */

#ifndef CARGO_IPC_INTERNALS_RESULT_BUILDER_HPP
#define CARGO_IPC_INTERNALS_RESULT_BUILDER_HPP

#include "cargo-ipc/result.hpp"
#include <functional>
#include <exception>
#include <memory>

namespace cargo {
namespace ipc {
namespace internals {

class ResultBuilder {
public:
    ResultBuilder()
        : mData(nullptr),
          mExceptionPtr(nullptr)
    {}

    explicit ResultBuilder(const std::exception_ptr& exceptionPtr)
        : mData(nullptr),
          mExceptionPtr(exceptionPtr)
    {}

    explicit ResultBuilder(const std::shared_ptr<void>& data)
        : mData(data),
          mExceptionPtr(nullptr)

    {}

    template<typename Data>
    Result<Data> build()
    {
        return Result<Data>(std::move(std::static_pointer_cast<Data>(mData)),
                            std::move(mExceptionPtr));
    }

private:
    std::shared_ptr<void> mData;
    std::exception_ptr mExceptionPtr;
};

typedef std::function<void(ResultBuilder&)> ResultBuilderHandler;


} // namespace internals
} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_INTERNALS_RESULT_BUILDER_HPP
