#include "http_server.hpp"
#include "base/macro.hpp"
#include "http_servlet.hpp"

void test()
{
    IM::http::HttpServer::ptr http_server(new IM::http::HttpServer);
    IM::Address::ptr addr = IM::Address::LookupAnyIpAddress("0.0.0.0:8020");
    while (!http_server->bind(addr))
    {
        sleep(2);
    }

    auto sb = http_server->getServletDispatch();
    sb->addServlet("/IM/xx", [](IM::http::HttpRequest::ptr req,
                                   IM::http::HttpResponse::ptr res,
                                   IM::http::HttpSession::ptr session)
                   { 
                    res->setBody(req->toString()); 
                    return 0; });

    sb->addGlobServlet("/IM/*", [](IM::http::HttpRequest::ptr req,
                                      IM::http::HttpResponse::ptr res,
                                      IM::http::HttpSession::ptr session)
                       { 
                        res->setBody("Glob: \r\n" + req->toString());
                        return 0; });

    http_server->start();
}

int main(int argc, char **argv)
{
    IM::IOManager iom(2);
    iom.schedule(test);
    return 0;
}