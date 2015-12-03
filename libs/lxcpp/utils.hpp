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
 * @author  Lukasz Pawelczyk (l.pawelczyk@samsung.com)
 * @brief   LXCPP utils headers
 */

#ifndef LXCPP_UTILS_HPP
#define LXCPP_UTILS_HPP

#include <string>


namespace lxcpp {


/**
 * Changes the tittle of a current process title (e.g. visible in ps tool).
 *
 * @param title  A new tittle to be set
 */
void setProcTitle(const std::string &title);

void setupMountPoints();

bool setupControlTTY(const int ttyFD);


} // namespace lxcpp


#endif // LXCPP_UTILS_HPP
