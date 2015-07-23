/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Mateusz Malicki <m.malicki2@samsung.com>
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
 * @brief   ContainerImpl definition
 */

#include <lxcpp/container-impl.hpp>
#include <lxcpp/exception.hpp>

namespace lxcpp {

ContainerImpl::ContainerImpl()
{
}

ContainerImpl::~ContainerImpl()
{
}

std::string ContainerImpl::getName()
{
    throw NotImplemented();
}

void ContainerImpl::setName(const std::string& /* name */)
{
    throw NotImplemented();
}

void ContainerImpl::start()
{
    throw NotImplemented();
}

void ContainerImpl::stop()
{
    throw NotImplemented();
}

void ContainerImpl::freeze()
{
    throw NotImplemented();
}

void ContainerImpl::unfreeze()
{
    throw NotImplemented();
}

void ContainerImpl::reboot()
{
    throw NotImplemented();
}

int ContainerImpl::getInitPid()
{
    throw NotImplemented();
}

void ContainerImpl::create()
{
    throw NotImplemented();
}

void ContainerImpl::destroy()
{
    throw NotImplemented();
}

void ContainerImpl::setRootPath(const std::string& /* path */)
{
    throw NotImplemented();
}

std::string getRootPath()
{
    throw NotImplemented();
}

} // namespace lxcpp
