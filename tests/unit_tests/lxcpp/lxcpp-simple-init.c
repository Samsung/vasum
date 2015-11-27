/*
 *  Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Lukasz Pawelczyk (l.pawelczyk@samsumg.com)
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
 * @author Lukasz Pawelczyk (l.pawelczyk@samsumg.com)
 * @brief  Simple and static container init
 */

#include <signal.h>
#include <unistd.h>

void sighandler(__attribute__((unused)) int signal)
{
    _exit(0);
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) const char *argv[])
{
    signal(SIGTERM, sighandler);
    signal(SIGUSR1, sighandler);
    signal(SIGUSR2, sighandler);

    sleep(60);

    return -1;
}
