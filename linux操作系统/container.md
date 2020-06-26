### Docker的基本原理

容器实现封闭的环境主要要靠两种技术，一种是看起来是隔离的技术，称为 namespace（命名空间）。在每个 namespace 中的应用看到的，都是不同的 IP 地址、用户空间、进程 ID 等。另一种是用起来是隔离的技术，称为 cgroup（网络资源限制），即明明整台机器有很多的 CPU、内存，但是一个应用只能用其中的一部分

安装docker：

```
https://docs.docker.com/engine/install/ubuntu/
```

搜索应用：

```
https://hub.docker.com/
```

```
// 下载应用
# docker pull ubuntu:14.04

// 查看镜像
# docker images
```

启动一个容器需要一个叫 entrypoint 的入口。一个容器启动起来之后，会从这个指令开始运行，并且只有这个指令在运行，容器才启动着。如果这个指令退出，整个容器就退出了

```
// entrypoint 设置为 bash，再查看系统信息
# docker run -it --entrypoint bash ubuntu:14.04
root@0e35f3f1fbc5:/# cat /etc/lsb-release 
DISTRIB_ID=Ubuntu
DISTRIB_RELEASE=14.04
DISTRIB_CODENAME=trusty
DISTRIB_DESCRIPTION="Ubuntu 14.04.6 LTS"
```



```
// 下载一个 nginx 的镜像
# docker pull nginx
Using default tag: latest
latest: Pulling from library/nginx
fc7181108d40: Pull complete 
d2e987ca2267: Pull complete 
0b760b431b11: Pull complete 
Digest: sha256:48cbeee0cb0a3b5e885e36222f969e0a2f41819a68e07aeb6631ca7cb356fed1
Status: Downloaded newer image for nginx:latest

// ，-d: daemon。冒号后面的 80 是容器内部环境监听的端口，冒号前面的 8080 是宿主机上监听的端口
# docker run -d -p 8080:80 nginx
73ff0c8bea6e169d1801afe807e909d4c84793962cba18dd022bfad9545ad488

// 查看都有哪些容器正在运行
# docker ps
CONTAINER ID        IMAGE               COMMAND                  CREATED             STATUS              PORTS                  NAMES
73ff0c8bea6e        nginx               "nginx -g 'daemon of…"   2 minutes ago       Up 2 minutes        0.0.0.0:8080->80/tcp   modest_payne

// 通过container ID查看容器信息
# docker inspect 73ff0c8bea6e

// 打印出 nginx 的欢迎页面
# curl http://localhost:8080
<!DOCTYPE html>
<html>
<head>
<title>Welcome to nginx!</title>
```

自己开发的应用打包成为镜像，要通过 Dockerfile，Dockerfile 的格式应该包含下面的部分：

```
FROM: 基础镜像
RUN: 运行过的所有命令
COPY: 拷贝到容器中的资源
ENTRYPOINT: 前台启动的命令或者脚本
```

如Dockerfile文件：

```

FROM ubuntu:14.04
RUN echo "deb http://archive.ubuntu.com/ubuntu trusty main restricted universe multiverse" > /etc/apt/sources.list
RUN echo "deb http://archive.ubuntu.com/ubuntu trusty-updates main restricted universe multiverse" >> /etc/apt/sources.list
RUN apt-get -y update
RUN apt-get -y install nginx
COPY test.html /usr/share/nginx/html/test.html
ENTRYPOINT nginx -g "daemon off;"
```

如test.html：

```
<!DOCTYPE html>
<html>
  <head>
    <title>Welcome to nginx Test 7!</title>
    <style>
      body {
        width: 35em;
        margin: 0 auto;
        font-family: Tahoma, Verdana, Arial, sans-serif;
      }
    </style>
  </head>
  <body>
    <h1>Test 7</h1>
    <p>If you see this page, the nginx web server is successfully installed and
    working. Further configuration is required.</p>
    <p>For online documentation and support please refer to
    <a href="http://nginx.org/">nginx.org</a>.<br/>
    Commercial support is available at
    <a href="http://nginx.com/">nginx.com</a>.</p>
    <p><em>Thank you for using nginx.</em></p>
  </body>
</html>
```

将代码、Dockerfile、脚本，放在一个文件夹下，编译这个 Dockerfile：

```
# docker build -f Dockerfile -t testnginx:1 .
```

查看镜像：

```
# docker images
```

运行镜像：

```
# docker run -d -p 8081:80 testnginx:1
f604f0e34bc263bc32ba683d97a1db2a65de42ab052da16df3c7811ad07f0dc3
# docker ps
CONTAINER ID        IMAGE               COMMAND                  CREATED             STATUS              PORTS                  NAMES
f604f0e34bc2        testnginx:1         "/bin/sh -c 'nginx -…"   2 seconds ago       Up 2 seconds        0.0.0.0:8081->80/tcp   youthful_torvalds
73ff0c8bea6e        nginx               "nginx -g 'daemon of…"   33 minutes ago      Up 33 minutes       0.0.0.0:8080->80/tcp   modest_payne
```

访问在 nginx 里面写的代码：

```
[root@deployer nginx]# curl http://localhost:8081/test.html
<!DOCTYPE html>
<html>
  <head>
    <title>Welcome to nginx Test 7!</title>
```

Dcoker对于CPU的限制：

*  -c --cpu-shares：Docker 允许用户为每个容器设置一个数字，默认每个容器是 1024。这个数值本身并不能代表任何确定的意义。当主机上有多个容器运行时，每个容器占用的 CPU 时间比例为它的 share 在总额中的比例
* --cpus：可以限定容器能使用的 CPU 核数
* --cpuset：让容器只运行在某些核上

Dcoker对于内存的限制：

* -m --memory：容器能使用的最大内存大小
* –memory-swap：容器能够使用的 swap 大小
* –memory-swappiness：默认情况下，主机可以把容器使用的匿名页 swap 出来，可以设置一个 0-100 之间的值，代表允许 swap 出来的比例
* –memory-reservation：设置一个内存使用的 soft limit，如果 docker 发现主机内存不足，会执行 OOM (Out of Memory) 操作。这个值必须小于 --memory 设置的值
* –kernel-memory：容器能够使用的 kernel memory 大小
* –oom-kill-disable：是否运行 OOM (Out of Memory) 的时候杀死容器。只有设置了 -m，才可以把这个选项设置为 false，否则容器会耗尽主机内存，而且导致主机应用被杀死

无论是容器，还是虚拟机，都依赖于内核中的技术，虚拟机依赖的是 KVM，容器依赖的是 namespace 和 cgroup 对进程进行隔离。为了运行 Docker，有一个 daemon 进程 Docker Daemon 用于接收命令行。为了描述 Docker 里面运行的环境和应用，有一个 Dockerfile，通过 build 命令称为容器镜像。容器镜像可以上传到镜像仓库，也可以通过 pull 命令从镜像仓库中下载现成的容器镜像。通过 Docker run 命令将容器镜像运行为容器，通过 namespace 和 cgroup 进行隔离，容器里面不包含内核，是共享宿主机的内核的。对比虚拟机，虚拟机在 qemu 进程里面是有客户机内核的，应用运行在客户机的用户态

![img](https://static001.geekbang.org/resource/image/5a/c5/5a499cb50a1b214a39ddf19cbb63dcc5.jpg)

### Namespace技术

Linux 内核里面实现了以下几种不同类型的 namespace：

* UTS，对应的宏为 CLONE_NEWUTS，表示不同的 namespace 可以配置不同的 hostname
* User：对应的宏为 CLONE_NEWUSER，表示不同的 namespace 可以配置不同的用户和组
* Mount：对应的宏为 CLONE_NEWNS，表示不同的 namespace 的文件系统挂载点是隔离的
* PID：对应的宏为 CLONE_NEWPID，表示不同的 namespace 有完全独立的 pid，也即一个 namespace 的进程和另一个 namespace 的进程，pid 可以是一样的，但是代表不同的进程
* Network：对应的宏为 CLONE_NEWNET，表示不同的 namespace 有独立的网络协议栈

 /proc/pid/ns 里面，能够看到这个进程所属于的 6 种 namespace，它们属于同一个 namespace：

```
# ls -l /proc/58212/ns 
lrwxrwxrwx 1 root root 0 Jul 16 19:19 ipc -> ipc:[4026532278]
lrwxrwxrwx 1 root root 0 Jul 16 19:19 mnt -> mnt:[4026532276]
lrwxrwxrwx 1 root root 0 Jul 16 01:43 net -> net:[4026532281]
lrwxrwxrwx 1 root root 0 Jul 16 19:19 pid -> pid:[4026532279]
lrwxrwxrwx 1 root root 0 Jul 16 19:19 user -> user:[4026531837]
lrwxrwxrwx 1 root root 0 Jul 16 19:19 uts -> uts:[4026532277]

# ls -l /proc/58253/ns 
lrwxrwxrwx 1 33 tape 0 Jul 16 19:20 ipc -> ipc:[4026532278]
lrwxrwxrwx 1 33 tape 0 Jul 16 19:20 mnt -> mnt:[4026532276]
lrwxrwxrwx 1 33 tape 0 Jul 16 19:20 net -> net:[4026532281]
lrwxrwxrwx 1 33 tape 0 Jul 16 19:20 pid -> pid:[4026532279]
lrwxrwxrwx 1 33 tape 0 Jul 16 19:20 user -> user:[4026531837]
lrwxrwxrwx 1 33 tape 0 Jul 16 19:20 uts -> uts:[4026532277]
```

namespace的常用操作命令：

1.  nsenter：用来运行一个进程，进入指定的 namespace

```
// 可以运行 /bin/bash，并且进入 nginx 所在容器的 namespace
# nsenter --target 58212 --mount --uts --ipc --net --pid -- env --ignore-environment -- /bin/bash

root@f604f0e34bc2:/# ip addr
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
23: eth0@if24: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default 
    link/ether 02:42:ac:11:00:03 brd ff:ff:ff:ff:ff:ff
    inet 172.17.0.3/16 brd 172.17.255.255 scope global eth0
       valid_lft forever preferred_lft forever
```

2. unshare：离开当前的 namespace，创建且加入新的 namespace，然后执行参数中指定的命令

```
// pid 和 net 都进入了新的 namespace
# unshare --mount --ipc --pid --net --mount-proc=/proc --fork /bin/bash

// 从 shell 上运行上面这行命令的话，好像没有什么变化，但是因为 pid 和 net 都进入了新的 namespace，所以查看进程列表和 ip 地址的时候应该会发现有所不同
// 看不到宿主机上的 IP 地址和网卡了
# ip addr
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00

// 看不到宿主机上的所有进程
# ps aux
USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root         1  0.0  0.0 115568  2136 pts/0    S    22:55   0:00 /bin/bash
root        13  0.0  0.0 155360  1872 pts/0    R+   22:55   0:00 ps aux
```

namespace的常用操作函数：

1. clone：创建一个新的进程，并把它放到新的 namespace 中

```
// 参数 flags可以设置为 CLONE_NEWUTS、CLONE_NEWUSER、CLONE_NEWNS、CLONE_NEWPID。CLONE_NEWNET 会将 clone 出来的新进程放到新的 namespace 中
int clone(int (*fn)(void *), void *child_stack, int flags, void *arg);
```

2. setns：将当前进程加入到已有的 namespace 中

```
// fd 指向 /proc/[pid]/ns/ 目录里相应 namespace 对应的文件，表示要加入哪个 namespace。nstype 用来指定 namespace 的类型，可以设置为 CLONE_NEWUTS、CLONE_NEWUSER、CLONE_NEWNS、CLONE_NEWPID 和 CLONE_NEWNET
int setns(int fd, int nstype);
```

3. unshare：使当前进程退出当前的 namespace，并加入到新创建的 namespace

```
// flags 用于指定一个或者多个上面的 CLONE_NEWUTS、CLONE_NEWUSER、CLONE_NEWNS、CLONE_NEWPID 和 CLONE_NEWNET
int unshare(int flags);
```

![img](https://static001.geekbang.org/resource/image/56/d7/56bb9502b58628ff3d1bee83b6f53cd7.png)

在内核里面，对于任何一个进程 task_struct 来讲，里面都会有一个成员 struct nsproxy，用于保存 namespace 相关信息，里面有 struct uts_namespace、struct ipc_namespace、struct mnt_namespace、struct pid_namespace、struct net *net_ns 和 struct cgroup_namespace *cgroup_ns。创建 namespace 的时候，在内核中会调用 copy_namespaces，调用顺序依次是 copy_mnt_ns、copy_utsname、copy_ipcs、copy_pid_ns、copy_cgroup_ns 和 copy_net_ns，来复制 namespace

### cgroup技术

control group：cgroup 定义了下面的一系列子系统，每个子系统用于控制某一类资源：

* CPU 子系统，主要限制进程的 CPU 使用率
* cpuacct 子系统，可以统计 cgroup 中的进程的 CPU 使用报告
* cpuset 子系统，可以为 cgroup 中的进程分配单独的 CPU 节点或者内存节点
* memory 子系统，可以限制进程的 Memory 使用量
* blkio 子系统，可以限制进程的块设备 IO
* devices 子系统，可以控制进程能够访问某些设备
* net_cls 子系统，可以标记 cgroups 中进程的网络数据包，然后可以使用 tc 模块（traffic control）对数据包进行控制
* freezer 子系统，可以挂起或者恢复 cgroup 中的进程

在 Linux 上，为了操作 cgroup，有一个专门的 cgroup 文件系统，运行 mount 命令可以查看

```
# mount -t cgroup
cgroup on /sys/fs/cgroup/systemd type cgroup (rw,nosuid,nodev,noexec,relatime,xattr,release_agent=/usr/lib/systemd/systemd-cgroups-agent,name=systemd)
cgroup on /sys/fs/cgroup/net_cls,net_prio type cgroup (rw,nosuid,nodev,noexec,relatime,net_prio,net_cls)
cgroup on /sys/fs/cgroup/perf_event type cgroup (rw,nosuid,nodev,noexec,relatime,perf_event)
cgroup on /sys/fs/cgroup/devices type cgroup (rw,nosuid,nodev,noexec,relatime,devices)
cgroup on /sys/fs/cgroup/blkio type cgroup (rw,nosuid,nodev,noexec,relatime,blkio)
cgroup on /sys/fs/cgroup/cpu,cpuacct type cgroup (rw,nosuid,nodev,noexec,relatime,cpuacct,cpu)
cgroup on /sys/fs/cgroup/memory type cgroup (rw,nosuid,nodev,noexec,relatime,memory)
cgroup on /sys/fs/cgroup/cpuset type cgroup (rw,nosuid,nodev,noexec,relatime,cpuset)
cgroup on /sys/fs/cgroup/hugetlb type cgroup (rw,nosuid,nodev,noexec,relatime,hugetlb)
cgroup on /sys/fs/cgroup/freezer type cgroup (rw,nosuid,nodev,noexec,relatime,freezer)
cgroup on /sys/fs/cgroup/pids type cgroup (rw,nosuid,nodev,noexec,relatime,pids)
```

cgroup 文件系统多挂载到 /sys/fs/cgroup 下，通过上面的命令行，可以用 cgroup 控制哪些资源

![img](https://static001.geekbang.org/resource/image/1c/0f/1c762a6283429ff3587a7fc370fc090f.png)

内核中 cgroup 的工作机制：

1. 初始化 cgroup 的各个子系统的操作函数，分配各个子系统的数据结构
2. mount cgroup 文件系统，创建文件系统的树形结构，以及操作函数
3. 写入 cgroup 文件，设置 cpu 或者 memory 的相关参数，这个时候文件系统的操作函数会调用到 cgroup 子系统的操作函数，从而将参数设置到 cgroup 子系统的数据结构中
4. 写入 tasks 文件，将进程交给某个 cgroup 进行管理，因为 tasks 文件也是一个 cgroup 文件，统一会调用文件系统的操作函数进而调用 cgroup 子系统的操作函数，将 cgroup 子系统的数据结构和进程关联起来
5. 对于 CPU 来讲，会修改 scheduled entity，放入相应的队列里面去，从而下次调度的时候就起作用了。对于内存的 cgroup 设定，只有在申请内存的时候才起作用

![img](https://static001.geekbang.org/resource/image/c9/c4/c9cc56d20e6a4bac0f9657e6380a96c4.png)

