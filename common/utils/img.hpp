/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Kostyra <l.kostyra@samsung.com>
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
 * @author  Lukasz Kostyra (l.kostyra@samsung.com)
 * @brief   Image utility functions declaration
 */

#ifndef COMMON_UTILS_IMG_HPP
#define COMMON_UTILS_IMG_HPP

namespace utils {

/**
 * Returns string with first free loop device.
 */
bool getFreeLoopDevice(std::string& ret);

/**
 * Mount an ext4 image from file on a given path by using a loop device.
 */
bool mountImage(const std::string& image, const std::string& loopdev, const std::string& path);

/**
 * Umounts previously mounted image.
 * This call will also free loop device used to mount specified path.
 */
bool umountImage(const std::string& path, const std::string& loopdev);

/**
 * Mounts an image and copies its contents to dst directory.
 */
bool copyImageContents(const std::string& img, const std::string& dst);

} // namespace utils

#endif // COMMON_UTILS_IMG_HPP
