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

### 

