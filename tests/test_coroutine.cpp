#include "base/macro.hpp"
#include "coroutine.hpp"

static auto g_logger = IM_LOG_ROOT();

void run_in_coroutine()
{
    IM_LOG_INFO(g_logger) << "run in coroutine begin";
    IM::Coroutine::GetThis()->YieldToHold(); // 从子协程返回主协程
    IM_LOG_INFO(g_logger) << "run in coroutine end";
    IM::Coroutine::GetThis()->YieldToHold();
}

void test_fiber()
{
    // 创建当前线程的主协程
    IM::Coroutine::GetThis();
    IM_LOG_INFO(g_logger) << "main begin -1";
    {
        IM_LOG_INFO(g_logger) << "main begin";
        // 创建子协程
        IM::Coroutine::ptr coroutine = std::make_shared<IM::Coroutine>(run_in_coroutine);
        coroutine->swapIn(); // 从当前线程的协程（主协程）进入子协程
        IM_LOG_INFO(g_logger) << "main after swapIn";
        coroutine->swapIn(); // 进入子协程
        IM_LOG_INFO(g_logger) << "main end";
        coroutine->swapIn();
    }
    IM_LOG_INFO(g_logger) << "main after 2";
}

int main(int argc, char **argv)
{
    IM::Thread::SetName("main");
    IM_LOG_INFO(g_logger) << "main";
    std::vector<IM::Thread::ptr> thrs;
    for(int i = 0; i < 5; ++i)
    {
        thrs.push_back(std::make_shared<IM::Thread>(&test_fiber,"thread_"+std::to_string(i)));
    }
    for(auto i : thrs)
    {
        i->join();
    }
    return 0;
}