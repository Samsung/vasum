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

#include "lxcpp/container-impl.hpp"
#include "lxcpp/exception.hpp"

namespace lxcpp {

ContainerImpl::ContainerImpl()
{
}

ContainerImpl::~ContainerImpl()
{
}

std::string ContainerImpl::getName()
{
    throw NotImplementedException();
}

void ContainerImpl::setName(const std::string& /* name */)
{
    throw NotImplementedException();
}

void ContainerImpl::start()
{
    throw NotImplementedException();
}

void ContainerImpl::stop()
{
    throw NotImplementedException();
}

void ContainerImpl::freeze()
{
    throw NotImplementedException();
}

void ContainerImpl::unfreeze()
{
    throw NotImplementedException();
}

void ContainerImpl::reboot()
{
    throw NotImplementedException();
}

int ContainerImpl::getInitPid()
{
    throw NotImplementedException();
}

void ContainerImpl::create()
{
    throw NotImplementedException();
}

void ContainerImpl::destroy()
{
    throw NotImplementedException();
}

void ContainerImpl::setRootPath(const std::string& /* path */)
{
    throw NotImplementedException();
}

std::string ContainerImpl::getRootPath()
{
    throw NotImplementedException();
}

} // namespace lxcpp
