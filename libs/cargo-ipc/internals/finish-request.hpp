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
 * @brief   Managing the queue with requests
 */

#ifndef CARGO_IPC_INTERNALS_FINISH_REQUEST_HPP
#define CARGO_IPC_INTERNALS_FINISH_REQUEST_HPP

#include <condition_variable>

namespace cargo {
namespace ipc {
namespace internals {

class FinishRequest {
public:
    FinishRequest(const FinishRequest&) = delete;
    FinishRequest& operator=(const FinishRequest&) = delete;

    FinishRequest(const std::shared_ptr<std::condition_variable>& conditionPtr)
        : conditionPtr(conditionPtr)
    {}

    std::shared_ptr<std::condition_variable> conditionPtr;
};

} // namespace internals
} // namespace ipc
} // namespace cargo

#endif // CARGO_IPC_INTERNALS_FINISH_REQUEST_HPP
