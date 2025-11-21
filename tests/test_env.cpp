#include "env.hpp"
#include <unistd.h>
#include <iostream>
#include <fstream>

struct A
{
    A()
    {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());

        for (size_t i = 0; i < content.size(); ++i)
        {
            std::cout << i << " - " << content[i] << " - " << (int)content[i] << std::endl;
        }
    }
};

//A a;

int main(int argc, char **argv)
{
    std::cout << "argc=" << argc << std::endl;
    IM::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    IM::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    IM::EnvMgr::GetInstance()->addHelp("p", "print help");
    if (!IM::EnvMgr::GetInstance()->init(argc, argv))
    {
        IM::EnvMgr::GetInstance()->printHelp();
        return 0;
    }

    std::cout << "exe=" << IM::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd=" << IM::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << IM::EnvMgr::GetInstance()->getEnv("PATH", "xxx") << std::endl;
    std::cout << "test=" << IM::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    std::cout << "set env " << IM::EnvMgr::GetInstance()->setEnv("TEST", "yy") << std::endl;
    std::cout << "test=" << IM::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    if (IM::EnvMgr::GetInstance()->has("p"))
    {
        IM::EnvMgr::GetInstance()->printHelp();
    }
    return 0;
}