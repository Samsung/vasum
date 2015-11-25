#include <unistd.h>
#include <iostream>
#include <vector>
#include <lxcpp/lxcpp.hpp>
#include <lxcpp/logger-config.hpp>
#include <lxcpp/network-config.hpp>
#include <logger/logger.hpp>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

using namespace lxcpp;

pid_t initPid = -1;

void sighandler(int signal)
{
    // remove the log in a deffered manner
    pid_t pid = fork();

    if (pid == 0)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            if (initPid > 0)
                ::kill(initPid, SIGTERM);
            sleep(11);
            ::unlink("/tmp/lxcpp-shell.txt");

            exit(0);
        }

        exit(0);
    }

    wait();
    exit(0);
}

// NOTE: to enable connecting outside the host from container (to be configured in upper layer):
// 1. enable ip forwarding (sysctl net.ipv4.....forwarding=1)
// 2. create iptables NAT/MASQ if required for that container

// For the example container configured in this file:
// --------------------------------------------------
// The commands issued on host to have internet access from container (masquarade)
// iptables -P FORWARD ACCEPT
// iptables -A POSTROUTING -t nat -j MASQUERADE -s 10.0.0.0/24
// --------------------------------------------------
// The commands issued on host to cleanup masquarade
// iptables -D POSTROUTING -t nat -j MASQUERADE -s 10.0.0.0/24

int main(int argc, char *argv[])
{
    if (getuid() != 0) {
        printf("Due to user namespace this program has to be run as root.\n");
        return 1;
    }

    signal(SIGINT, &sighandler);

    logger::setupLogger(logger::LogType::LOG_STDERR, logger::LogLevel::TRACE);
    LOGT("Color test: TRACE");
    LOGD("Color test: DEBUG");
    LOGI("Color test: INFO");
    LOGW("Color test: WARN");
    LOGE("Color test: ERROR");

    logger::setupLogger(logger::LogType::LOG_STDERR, logger::LogLevel::DEBUG);
    // logger::setupLogger(logger::LogType::LOG_STDERR, logger::LogLevel::TRACE);
    // logger::setupLogger(logger::LogType::LOG_FILE, logger::LogLevel::TRACE, "/tmp/lxcpp-shell.txt");

    std::vector<std::string> args;
    args.push_back("/bin/bash");

    try
    {
        Container* c = createContainer("test", "/", "/tmp");
        c->setHostName("junk");
        c->setInit(args);
        c->setLogger(logger::LogType::LOG_FILE, logger::LogLevel::TRACE, "/tmp/lxcpp-shell.txt");
        c->setTerminalCount(4);
        // make my own user root in a new namespace
        c->addUIDMap(0, 1000, 1);
        c->addGIDMap(0, 1000, 1);
        // make root and system users non privileged ones
        c->addUIDMap(1000, 0, 999);
        c->addGIDMap(1000, 0, 999);

        // configure network
        c->addInterfaceConfig(InterfaceConfigType::LOOPBACK, "lo");
        c->addInterfaceConfig(InterfaceConfigType::BRIDGE, "lxcpp-br0", "", {InetAddr("10.0.0.1", 24)});
        c->addInterfaceConfig(InterfaceConfigType::VETH_BRIDGED, "lxcpp-br0", "veth0", {InetAddr("10.0.0.2", 24)});

        c->start();
        // not needed per se, but let things settle for a second, e.g. the logs
        sleep(1);
        c->console();
        // You could run the console for the second time to see if it can be reattached
        //c->console();

        delete c;
    }
    catch (const std::exception &e)
    {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
    }

    sighandler(3);

    return 0;
}
