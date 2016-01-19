/*
 *  Copyright (c) 2016 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Maciej Karpiuk (m.karpiuk2@samsumg.com)
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
 * @author Maciej Karpiuk (m.karpiuk2@samsumg.com)
 * @brief  Simple and static rand gen
 */

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv)
{
    int i, out_fd, ec = 0;

    if (argc < 3) {
        return -1;
    }

    srand(time(NULL));

    out_fd = open(argv[2], O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (out_fd < 0) {
        return -2;
    }

    for (i = 0; i < atoi(argv[1]); i++) {
        char buf[16];
        snprintf(buf, sizeof(buf)-1, "%d", rand());
        if (write(out_fd, buf, strlen(buf)) != (int)strlen(buf)) {
            ec = -3;
            break;
        }
    }
    close(out_fd);

    return ec;
}
