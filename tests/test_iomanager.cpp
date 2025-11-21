#include "base/macro.hpp"
#include "iomanager.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

auto g_logger = IM_LOG_ROOT();

void test_coroutine()
{
    IM_LOG_INFO(g_logger) << "test_coroutine";
}

void test1()
{
    IM::IOManager iom(2, false, "test");
    iom.schedule(test_coroutine);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(fd, F_SETFL, O_NONBLOCK);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "192.168.126.100", &addr.sin_addr.s_addr);

    connect(fd, (const sockaddr *)&addr, sizeof(addr));
    iom.addEvent(fd, IM::IOManager::WRITE, []()
                 { IM_LOG_INFO(g_logger) << "write callback"; });
    iom.addEvent(fd, IM::IOManager::READ, []()
                 { IM_LOG_INFO(g_logger) << "read callback"; });
}

void test_timer()
{
    static IM::Timer::ptr timer;
    IM::IOManager iom(2, false, "test");
    timer = iom.addTimer(
        1000, []()
        { 
            static int i=0;
            IM_LOG_INFO(g_logger) << "timeout i = " << i ;
            if(++i == 5)
            {
                //timer->cancel();
                timer->reset(2000, true);
            } },
        true);
}

int main(int argc, char **argv)
{
    test_timer();
    return 0;
}