/*
 *  Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Jan Olszak (j.olszak@samsung.com)
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
 * @brief   Helper function for iterating tuples, pairs and arrays
 */


#ifndef CARGO_INTERNALS_VISIT_FIELDS_HPP
#define CARGO_INTERNALS_VISIT_FIELDS_HPP

#include <string>
#include <tuple>

namespace {

template<int I, class T, class F, typename ... A>
struct visitImpl
{
    static void visit(T& t, F f, A&& ... args)
    {
        visitImpl<I - 1, T, F, A...>::visit(t, f, std::forward<A>(args)...);
        f->visit(args..., std::get<I>(t));
    }
};

template<class T, class F, typename ... A>
struct visitImpl<0, T, F, A...>
{
    static void visit(T& t, F f, A&& ... args)
    {
        f->visit(args..., std::get<0>(t));
    }
};

} // namespace

namespace cargo {
namespace internals {

template<class T, class F, typename ... A>
void visitFields(T& t, F f, A ... args)
{
    visitImpl<std::tuple_size<T>::value - 1, T, F, A...>::visit(t, f, std::forward<A>(args)...);
}

} // namespace internals
} // namespace cargo

#endif // CARGO_INTERNALS_VISIT_FIELDS_HPP
