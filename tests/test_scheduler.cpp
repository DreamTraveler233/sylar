#include "base/macro.hpp"
#include "scheduler.hpp"
#include"iomanager.hpp"

auto g_logger = IM_LOG_ROOT();

void test_fiber()
{
    static int count = 5;
    IM_LOG_INFO(g_logger) << "test begin count=" << count;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (--count>0)
    {
        IM::IOManager::GetThis()->schedule(test_fiber);
    }
}

int main(int argc, char **argv)
{
    // IM::Scheduler sc(1,true,"test");
    // sc.start();
    // sc.schedule(test_fiber);
    // sc.stop();

    IM::IOManager iom(2,true,"test");
    iom.schedule(test_fiber);
    return 0;
}