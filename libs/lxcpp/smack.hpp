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
 * @brief   SMACK utils headers
 */

#ifndef LXCPP_SMACK_HPP
#define LXCPP_SMACK_HPP

#include <string>

#define SMACK_MOUNT_PATH "/sys/fs/smackfs"
#define SMACK_LABEL_MAX_LEN 255

#ifndef SMACK_MAGIC
#define SMACK_MAGIC 0x43415d53 // "SMAC"
#endif


namespace lxcpp {


enum class SmackLabelType : int {
    SMACK_LABEL_ACCESS = 0,
    SMACK_LABEL_EXEC,
    SMACK_LABEL_MMAP,
    SMACK_LABEL_TRANSMUTE,
    SMACK_LABEL_IPIN,
    SMACK_LABEL_IPOUT,
};

bool isSmackActive();
bool isSmackNamespaceActive();
std::string smackXattrName(SmackLabelType type);
std::string smackGetSelfLabel();
std::string smackGetFileLabel(const std::string &path,
                              SmackLabelType labelType,
                              int followLinks);
void smackSetFileLabel(const std::string &path,
                       const std::string &label,
                       SmackLabelType labelType,
                       int followLinks);


} // namespace lxcpp


#endif // LXCPP_SMACK_HPP
