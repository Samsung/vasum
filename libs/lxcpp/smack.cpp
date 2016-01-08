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
 * @brief   SMACK utils implementation
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "config.hpp"

#include "lxcpp/exception.hpp"
#include "lxcpp/smack.hpp"

#include "logger/logger.hpp"
#include "utils/fs.hpp"

#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/xattr.h>


namespace lxcpp {


bool isSmackActive()
{
    struct ::statfs sfbuf;
    int rc;

    do {
        rc = ::statfs(SMACK_MOUNT_PATH, &sfbuf);
    } while (rc < 0 && errno == EINTR);

    if (rc == 0 && (uint32_t)sfbuf.f_type == (uint32_t)SMACK_MAGIC)
        return true;

    return false;
}

bool isSmackNamespaceActive()
{
    return utils::exists("/proc/self/attr/label_map");
}

std::string smackXattrName(SmackLabelType type)
{
    switch (type) {
    case SmackLabelType::SMACK_LABEL_ACCESS:
        return "security.SMACK64";
    case SmackLabelType::SMACK_LABEL_EXEC:
        return "security.SMACK64EXEC";
    case SmackLabelType::SMACK_LABEL_MMAP:
        return "security.SMACK64MMAP";
    case SmackLabelType::SMACK_LABEL_TRANSMUTE:
        return "security.SMACK64TRANSMUTE";
    case SmackLabelType::SMACK_LABEL_IPIN:
        return "security.SMACK64IPIN";
    case SmackLabelType::SMACK_LABEL_IPOUT:
        return "security.SMACK64IPOUT";
    default: {
        const std::string msg = "Wrong SMACK label type passed";
        LOGE(msg);
        throw SmackException(msg);
    }
    }
}

std::string smackGetSelfLabel()
{
    return utils::readFileStream("/proc/self/attr/current");
}

std::string smackGetFileLabel(const std::string &path,
                              SmackLabelType labelType,
                              int followLinks)
{
    char value[SMACK_LABEL_MAX_LEN + 1];
    int ret;
    const std::string xattrName = smackXattrName(labelType);

    if (followLinks) {
        ret = ::getxattr(path.c_str(), xattrName.c_str(), value,
                         SMACK_LABEL_MAX_LEN + 1);
    } else {
        ret = ::lgetxattr(path.c_str(), xattrName.c_str(), value,
                          SMACK_LABEL_MAX_LEN + 1);
    }

    if (ret == -1) {
        if (errno == ENODATA) {
            return "";
        }

        const std::string msg = "Failed to get SMACK label";
        LOGE(msg);
        throw SmackException(msg);
    }

    value[ret] = '\0';
    return value;
}

void smackSetFileLabel(const std::string &path,
                       const std::string &label,
                       SmackLabelType labelType,
                       int followLinks)
{
    int ret = -1;
    int len = label.size();
    const std::string xattrName = smackXattrName(labelType);

    if (len > SMACK_LABEL_MAX_LEN) {
        const std::string msg = "SMACK label too long";
        LOGE(msg);
        throw SmackException(msg);
    }

    if (label.empty()) {
        if (followLinks) {
            ret = ::removexattr(path.c_str(), xattrName.c_str());
        } else {
            ret = ::lremovexattr(path.c_str(), xattrName.c_str());
        }

        if (ret == -1 && errno == ENODATA)
            ret = 0;
    } else {
        if (followLinks) {
            ret = ::setxattr(path.c_str(), xattrName.c_str(), label.c_str(), len, 0);
        } else {
            ret = ::lsetxattr(path.c_str(), xattrName.c_str(), label.c_str(), len, 0);
        }
    }

    if (ret < 0) {
        const std::string msg = "Failed to set SMACK label";
        LOGE(msg);
        throw SmackException(msg);
    }
}


} // namespace lxcpp
