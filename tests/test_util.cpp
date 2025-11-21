#include "base/macro.hpp"
#include "util.hpp"
#include "base/macro.hpp"
#include <assert.h>

auto g_logger = IM_LOG_ROOT();

void test_assert()
{
    IM_LOG_ERROR(g_logger) << IM::BacktraceToString(100);
    IM_ASSERT(false);
}

int main(int argc, char **argv)
{
    test_assert();
    return 0;
}