/*
 *  Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Contact: Bumjin Im <bj.im@samsung.com>
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
 * @file    main.cpp
 * @author  Jan Olszak (j.olszak@samsung.com)
 * @brief   Main file for the Security Containers Daemon
 */


#include <iostream>
#include <getopt.h>             // For getopt

int main(int argc, char *argv[])
{
    int optIndex = 0;

    const option longOptions[] = {
        {"help",    no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    for (;;) {
        int opt = getopt_long(argc, argv,
                              "hv", // ':' after arg is the parameter
                              longOptions,
                              &optIndex);
        if (opt == -1) {
            break;
        }

        // If option comes with a parameter,
        // the param is stored in optarg global variable by getopt_long.
        switch (opt) {
        case 0:
            // A flag was set
            break;

        case '?':
            // No such command.
            // getopt_long already printed an error message to stderr.
            return 1;

        case 'v':
            std::cout << "Security Containers Server v. 0.1.0" << std::endl;
            return 0;

        case 'h':
            std::cout << "Security Containers Server v. 0.1.0          \n"
                      << "    Options:                                 \n"
                      << "        -h,--help     print this help        \n"
                      << "        -v,--version  show applcation version"
                      << std::endl;
            return 0;

        default:
            break;
        }
    }

    // Print unknown remaining command line arguments
    if (optind < argc) {
        std::cerr << "Unknown options: ";
        while (optind < argc) {
            std::cerr << argv[optind++] << " ";
        }
        std::cerr << std::endl;
        return 1;
    }
}