/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
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
 * @author  Lukasz Pawelczyk (l.pawelczyk@partner.samsung.com)
 * @brief   Configuration file for the code
 */


#ifndef COMMON_CONFIG_HPP
#define COMMON_CONFIG_HPP


#ifdef __clang__
#define CLANG_VERSION (__clang__major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#endif // __clang__

#if defined __GNUC__ && !defined __clang__  // clang also defines GCC versions
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif // __GNUC__


#ifdef GCC_VERSION

#if GCC_VERSION < 40800
// GCC 4.8 is the first where those defines are not required for
// std::this_thread::sleep_for() and ::yield(). They might exist though
// in previous versions depending on the build configuration of the GCC.
#ifndef _GLIBCXX_USE_NANOSLEEP
#define _GLIBCXX_USE_NANOSLEEP
#endif // _GLIBCXX_USE_NANOSLEEP
#ifndef _GLIBCXX_USE_SCHED_YIELD
#define _GLIBCXX_USE_SCHED_YIELD
#endif // _GLIBCXX_USE_SCHED_YIELD
#endif // GCC_VERSION < 40800

#if GCC_VERSION < 40700
// Those appeared in 4.7 with full c++11 support
#define final
#define override
#define thread_local __thread  // use GCC extension instead of C++11
#define steady_clock monotonic_clock
#endif // GCC_VERSION < 40700

#endif // GCC_VERSION

// Variadic macros support for boost preprocessor should be enabled
// manually for clang since they are marked as untested feature
// (boost trunk if fixed but the latest 1.55 version is not,
// see boost/preprocessor/config/config.hpp)
#ifdef __clang__
#define BOOST_PP_VARIADICS 1
#endif

// This has to be defined always when the boost has not been compiled
// using C++11. Headers detect that you are compiling using C++11 and
// blindly and wrongly assume that boost has been as well.
#ifndef BOOST_NO_CXX11_SCOPED_ENUMS
#define BOOST_NO_CXX11_SCOPED_ENUMS 1
#endif

#endif // COMMON_CONFIG_HPP
