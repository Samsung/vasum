/*
 *  Copyright (C) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
