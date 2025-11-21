#include "http.hpp"
#include "base/macro.hpp"
#include <iostream>

static auto g_logger = IM_LOG_ROOT();

void test_request()
{
    IM::http::HttpRequest::ptr req(new IM::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    std::cout << req->toString() << std::endl;
}

void test_response()
{
    IM::http::HttpResponse::ptr req(new IM::http::HttpResponse);
    req->setHeader("X-X", "IM");
    req->setBody("hello world");
    req->setStatus((IM::http::HttpStatus)404);
    req->setClose(false);
    std::cout << req->toString() << std::endl;
}

int main(int argc, char **crgv)
{
    test_request();
    test_response();
    return 0;
}