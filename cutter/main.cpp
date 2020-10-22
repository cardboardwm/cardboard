// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include <cstdlib>
#include <iostream>

#include <cardboard/client.h>
#include <cardboard/command_protocol.h>
#include <cardboard/ipc.h>

#include "parse_arguments.h"

void print_usage(char* argv0)
{
    std::cerr << "Usage: " << argv0 << " <command> [args...]" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    CommandData command_data = parse_arguments(argc, argv)
                                   .map_error([](const std::string& error) {
                                       std::cerr << "cutter: " << error << std::endl;
                                       exit(EXIT_FAILURE);
                                   })
                                   .value();

    libcutter::Client client = libcutter::open_client()
                                   .map_error([](const std::string& error) {
                                       std::cerr << "cutter: " << error << std::endl;
                                       exit(EXIT_FAILURE);
                                   })
                                   .value();

    client.send_command(command_data)
        .map_error([](const std::string& error) {
            std::cerr << "cutter: " << error << std::endl;
            exit(EXIT_FAILURE);
        });

    std::string response = client.wait_response()
                               .map_error([](int error_code) {
                                   std::cerr << "cutter: error code " << error_code << std::endl;
                                   exit(EXIT_FAILURE);
                               })
                               .value();

    std::cout << response;

    return 0;
}
