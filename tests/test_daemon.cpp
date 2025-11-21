#include "daemon.hpp"
#include "iomanager.hpp"
#include "base/macro.hpp"

static IM::Logger::ptr g_logger = IM_LOG_ROOT();

IM::Timer::ptr timer;
int server_main(int argc, char** argv) {
    IM_LOG_INFO(g_logger) << IM::ProcessInfoMgr::GetInstance()->toString();
    IM::IOManager iom(1);
    timer = iom.addTimer(1000, [](){
            IM_LOG_INFO(g_logger) << "onTimer";
            // static int count = 0;
            // if(++count > 10) {
            //     exit(1);
            // }
    }, true);
    return 0;
}

int main(int argc, char** argv) {
    return IM::start_daemon(argc, argv, server_main, argc != 1);
}