FROM ubuntu:25.04

# 避免安装过程中的交互式弹窗打断
ENV DEBIAN_FRONTEND=noninteractive

# 安装 CS144 编译及 Lab 4 网络虚拟网卡所需的全部底层工具
RUN apt-get update && apt-get install -y \
    sudo git cmake gdb build-essential \
    clang clang-tidy clang-format \
    gcc-doc pkg-config glibc-doc \
    tcpdump tshark iproute2 iptables \
    && rm -rf /var/lib/apt/lists/*

# 容器内启动时默认进入 bash
CMD ["/bin/bash"]
