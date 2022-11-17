# 使用变量 SRC_LIST 存储这两个源文件
set(SRC_LIST userservice.cpp ../user.pb.cc)
# 根据 SRC_LIST 中的源文件生成可执行文件
add_executable(provider ${SRC_LIST})
# provider 的编译需要使用框架代码，所以将其链接到 mprpc 库，还要链接到 protobuf 库
target_link_libraries(provider mprpc protobuf)