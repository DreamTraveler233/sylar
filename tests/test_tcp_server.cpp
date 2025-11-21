#include "tcp_server.hpp"
#include "iomanager.hpp"
#include "base/macro.hpp"

IM::Logger::ptr g_logger = IM_LOG_ROOT();

void run()
{
    auto addr = IM::Address::LookupAny("0.0.0.0:8033");
    IM_LOG_INFO(g_logger) << *addr;
    //auto addr2 = IM::UnixAddress::ptr(new IM::UnixAddress("/tmp/unix_addr"));
    std::vector<IM::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    IM::TcpServer::ptr tcp_server(new IM::TcpServer);
    std::vector<IM::Address::ptr> fails;
    while (!tcp_server->bind(addrs, fails))
    {
        sleep(2);
    }
    tcp_server->start();
}
int main(int argc, char **argv)
{
    IM::IOManager iom(2);
    iom.schedule(run);
    return 0;
}