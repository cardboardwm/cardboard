#include <cstdlib>
#include <iostream>

#include <cardboard/command_protocol.h>
#include <cardboard/ipc.h>
#include <cutter/client.h>

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
