#ifndef SECURITY_CONTAINERS_CLIENT_EXCEPTION_HPP
#define SECURITY_CONTAINERS_CLIENT_EXCEPTION_HPP

#include <stdexcept>

namespace security_containers {

/**
 * @brief Base class for exceptions in Security Containers Client
 */
struct ClientException: public std::runtime_error {
    ServerException(const std::string& mess = "Security Containers Client Exception"):
        std::runtime_error(mess) {};
};

}

#endif // SECURITY_CONTAINERS_CLIENT_EXCEPTION_HPP
