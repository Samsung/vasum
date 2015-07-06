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

#ifndef COMMON_RESULT_IPC_METHOD_RESULT_BUILDER_HPP
#define COMMON_RESULT_IPC_METHOD_RESULT_BUILDER_HPP

#include "config.hpp"

#include "api/method-result-builder.hpp"
#include "ipc/method-result.hpp"

#include <memory>

namespace vasum {
namespace api {

const std::string IPC_CONNECTION_PREFIX = "ipc://";

class IPCMethodResultBuilder: public MethodResultBuilder {
public:
    IPCMethodResultBuilder(const ipc::MethodResult::Pointer& methodResult);
    ~IPCMethodResultBuilder() {}

private:
    void setImpl(const std::shared_ptr<void>& data) override;
    void setVoid() override;
    void setError(const std::string& name, const std::string& message) override;
    std::string getID() const override;

    ipc::MethodResult::Pointer mMethodResultPtr;
};


} // namespace result
} // namespace vasum

#endif // COMMON_RESULT_IPC_METHOD_RESULT_BUILDER_HPP
