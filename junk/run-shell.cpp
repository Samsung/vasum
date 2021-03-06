#include <unistd.h>
#include <iostream>
#include <vector>
#include <lxcpp/lxcpp.hpp>
#include <lxcpp/logger-config.hpp>
#include <lxcpp/network-config.hpp>
#include <lxcpp/cgroups/cgroup.hpp>
#include <logger/logger.hpp>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>

#define WORK_DIR "/run/lxcpp"
#define CONT_DIR "/var/lib/lxc/lxcpp/rootfs"


using namespace lxcpp;

pid_t initPid = -1;

void sighandler(int signal)
{
    // remove the log in a deffered manner
    pid_t pid = ::fork();

    if (pid == 0)
    {
        pid_t pid = ::fork();

        if (pid == 0)
        {
            if (initPid > 0)
                ::kill(initPid, SIGTERM);
            ::sleep(20);
            ::unlink("/tmp/lxcpp-impl.txt");
            ::unlink("/tmp/lxcpp-guard.txt");

            ::exit(0);
        }

        ::exit(0);
    }

    ::wait();
    ::exit(0);
}

// NOTE: to enable connecting outside the host from container (to be configured in upper layer):
// 1. enable ip forwarding (sysctl net.ipv4.....forwarding=1)
// 2. create iptables NAT/MASQ if required for that container

// For the example container configured in this file:
// --------------------------------------------------
// The commands issued on host to have internet access from container (masquarade)
// iptables -A FORWARD -j ACCEPT -s 10.0.0.0/24
// iptables -A POSTROUTING -t nat -j MASQUERADE -s 10.0.0.0/24
// --------------------------------------------------
// The commands issued on host to cleanup masquarade
// iptables -D POSTROUTING -t nat -j MASQUERADE -s 10.0.0.0/24
// iptables -D FORWARD -j ACCEPT -s 10.0.0.0/24

int main(int argc, char *argv[])
{
    if (::getuid() != 0) {
        ::printf("Due to user namespace this program has to be run as root.\n");
        return 1;
    }

    ::signal(SIGINT, &sighandler);

    logger::setupLogger(logger::LogType::LOG_STDERR, logger::LogLevel::TRACE);
    LOGT("Color test: TRACE");
    LOGD("Color test: DEBUG");
    LOGI("Color test: INFO");
    LOGW("Color test: WARN");
    LOGE("Color test: ERROR");

    logger::setupLogger(logger::LogType::LOG_STDERR, logger::LogLevel::DEBUG);
    //logger::setupLogger(logger::LogType::LOG_FILE, logger::LogLevel::DEBUG, "/tmp/lxcpp-impl.txt");

    std::vector<std::string> args;
    args.push_back("/bin/bash"); args.push_back("--login");
    //args.push_back("/bin/login"); args.push_back("root");
    //args.push_back("/usr/lib/systemd/systemd");

    ::mkdir(WORK_DIR, 0755);

    try
    {
        Container *c = createContainer("test", CONT_DIR, WORK_DIR);

        c->setHostName("junk");
        c->setInit(args);
        c->setEnv({{"TEST_VAR", "test_value"}, {"_TEST_VAR_", "_test_value_"}, {"TERM", "xterm"}});
                  //{"LIBVIRT_LXC_UUID", "94e491f6-a13a-4871-abd9-362b999b3928"}, {"container_uuid", "94e491f6-a13a-4871-abd9-362b999b3928"},
                  //{"LIBVIRT_LXC_NAME", "test"}, {"container", "lxc-libvirt"}});
        c->setLogger(logger::LogType::LOG_PERSISTENT_FILE, logger::LogLevel::DEBUG, "/tmp/lxcpp-guard.txt");
        c->setTerminalCount(4);

        // make my own user root in a new namespace
        c->addUIDMap(0, 1000, 1000);
        c->addGIDMap(0, 1000, 1000);

        // configure network
        c->addInterfaceConfig(InterfaceConfigType::LOOPBACK, "lo");
        c->addInterfaceConfig(InterfaceConfigType::BRIDGE, "lxcpp-br0", "", {InetAddr("10.0.0.1", 24)});
        c->addInterfaceConfig(InterfaceConfigType::VETH_BRIDGED, "lxcpp-br0", "veth0", {InetAddr("10.0.0.2", 24)});

        // configure cgroups
        if (Subsystem("systemd").isAttached()) {
            c->addCGroup("systemd", "lxcpp/" + c->getName(), {}, {});
        }
        std::vector<std::string> subsystems = lxcpp::Subsystem::availableSubsystems();
        for (const auto &subsystem : subsystems) {
            c->addCGroup(subsystem, "lxcpp/" + c->getName(), {}, {});
        }

        // configure resource limits and kernel parameters
        c->setRlimit(RLIMIT_CPU, RLIM_INFINITY, RLIM_INFINITY);
        c->setRlimit(RLIMIT_DATA, 512 * 1024, 1024 * 1024);
        c->setKernelParameter("net.ipv6.conf.veth0.disable_ipv6", "1");

        c->start();
        // not needed per se, but let things settle for a second, e.g. the logs
        ::sleep(1);

        if (c->getState() == Container::State::RUNNING) {
            c->console();

            // You could run the console for the second time to see if it can be reattached
            //c->console();
        }

        //c->attach({"/usr/bin/sleep", "60"}, 0, 0, "", {}, 0, "/tmp", {}, {{"TEST_VAR","test_value"}});

        delete c;

        // Test reconnect
        c = createContainer("test", CONT_DIR, WORK_DIR);
        c->connect();
        ::sleep(1);

        if (c->getState() == Container::State::RUNNING) {
            c->console();

            // Stop it now, as it doesn't automatically on destructor
            c->stop();
        }

        delete c;
    }
    catch (const std::exception &e)
    {
        std::cout << "EXCEPTION: " << e.what() << std::endl;
    }

    ::sighandler(3);

    return 0;
}
