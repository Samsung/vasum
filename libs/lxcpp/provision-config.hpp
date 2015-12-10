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
 * @author  Maciej Karpiuk (m.karpiuk2@samsung.com)
 * @brief   Provisioning configuration
 */

#ifndef LXCPP_PROVISION_CONFIG_HPP
#define LXCPP_PROVISION_CONFIG_HPP

#include "cargo/fields.hpp"
#include "cargo/fields-union.hpp"

#include <vector>
#include <string>
#include <string.h>

namespace lxcpp {
namespace provision {

typedef std::string ProvisionID;

/**
 * Provision configuration items
 */
struct File
{
    enum class Type : int
    {
        DIRECTORY,
        FIFO,
        REGULAR
    };

    ProvisionID getId() const
    {
        return "file " +
               path + " " +
               std::to_string(static_cast<int>(type)) + " " +
               std::to_string(flags) + " " +
               std::to_string(mode);
    }

    Type type;
    std::string path;
    std::int32_t flags;
    std::int32_t mode;

    CARGO_REGISTER
    (
        type,
        path,
        flags,
        mode
    )

    bool operator==(const File& m) const
    {
        return ((m.type == type) && (m.path == path) &&
                (m.flags == flags) && (m.mode == mode));
    }
};

struct Mount
{
    ProvisionID getId() const
    {
        return "mount " +
               source + " " +
               target + " " +
               type + " " +
               std::to_string(flags) + " " +
               data;
    }

    std::string source;
    std::string target;
    std::string type;
    std::int64_t flags;
    std::string data;

    CARGO_REGISTER
    (
        source,
        target,
        type,
        flags,
        data
    )

    bool operator==(const Mount& m) const
    {
        return ((m.source == source) && (m.target == target) && (m.type == type) &&
                (m.flags == flags) && (m.data == data));
    }
};

struct Link
{
    ProvisionID getId() const
    {
        return "link " + source + " " + target;
    }

    std::string source;
    std::string target;

    CARGO_REGISTER
    (
        source,
        target
    )

    bool operator==(const Link& m) const
    {
        return ((m.source == source) && (m.target == target));
    }
};
} // namespace provision

typedef std::vector<provision::File>   FileVector;
typedef std::vector<provision::Mount>  MountVector;
typedef std::vector<provision::Link>   LinkVector;

struct ProvisionConfig
{
    FileVector files;
    MountVector mounts;
    LinkVector links;

    void addFile(const provision::File& newFile);
    const FileVector& getFiles() const;
    void removeFile(const provision::File& item);

    void addMount(const provision::Mount& newMount);
    const MountVector& getMounts() const;
    void removeMount(const provision::Mount& item);

    void addLink(const provision::Link& newLink);
    const LinkVector& getLinks() const;
    void removeLink(const provision::Link& item);

    CARGO_REGISTER
    (
        files,
        mounts,
        links
    )
};

} // namespace lxcpp

#endif // LXCPP_PROVISION_CONFIG_HPP
