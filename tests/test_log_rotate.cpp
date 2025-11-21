#include "base/macro.hpp"
#include "iomanager.hpp"
#include "config.hpp"

void test()
{
    static auto g_logger = IM_LOG_NAME("system");

    YAML::Node test = YAML::LoadFile("/home/szy/code/CIM/CIM_B/bin/config/log.yaml");
    IM::Config::LoadFromYaml(test);

    // IM::FileLogAppender::ptr file_ap(new IM::FileLogAppender("/home/szy/code/CIM/CIM_B/bin/log/test.log"));
    // file_ap->getLogFile()->setRotateType(IM::RotateType::Minute);
    // g_logger->addAppender(file_ap);

    for (int i = 0; i < 1000000; ++i)
    {
        IM_LOG_INFO(g_logger) << "第 " << i << " 条日志";
        //sleep(0.1);
    }

    // // 加载日志配置文件
    // YAML::Node root = YAML::LoadFile("/home/szy/code/CIM/CIM_B/bin/log/log.yaml");
    // IM::Config::LoadFromYaml(root);
}

int main(int argc, char **argv)
{
    IM::IOManager iom(2);
    iom.schedule(test);
    return 0;
}