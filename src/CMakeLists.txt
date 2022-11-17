# 查找当前目录下的所有源文件，并将文件名保存到 SRC_LIST 变量
aux_source_directory(. SRC_LIST)
# 使用 SRC_LIST 中的源文件生成一个名为 mprpc 的动态库(库文件的输出路径已经在顶级的CMakeLists.txt文件中指定好了)
add_library(mprpc ${SRC_LIST})
# 添加依赖的网络库，muduo_net和muduo_base 的顺序不能换，因为muduo_base也依赖了muduo_net
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)