### 软件安装方式

1. 安装包：

```
install:
CentOS: rpm -i *.rmp
Ubuntu: dpkg -i *.deb

uninstall:
CentOS: rpm -e *.rmp
Ubuntu: dpkg -r *.deb

search: 
CentOS: rpm -qa | grep keyword
Ubuntu: dpkg -I | grep keyword

dpkg -S softwarename: 显示包含此软件包的所有位置
dpkg -L softwarename: 显示安装路径
```

2. 软件管家：

```
install:
CentOS: yum install java-11-openjdk.x86
Ubuntu: apt-get install openjdk-9-jdk

uninstall: 
CentOS: yum erase java-11-openjdk.x86
Ubuntu: apt-get purge openjdk-9-jdk
```

软件源路径：

```
/etc/apt/sources.list
```

3. 下载解压：

```
wget wget http://www.baidu.com
```

将安装好路径的软件包下载后的tar.gz或者zip解压即可，无需安装，但是需要配置环境变量

### 环境变量路径

1. vim ~/.bashrc // 仅对当前用户有效
2. vim ~/.profile // 仅对当前用户有效
3. vim /etc/profile // 对所有用用有效
4. vim /etc/environment // 对所有用用有效

### 查看环境变量

- 查看单个环境变量: echo $PATH
- 查看所有环境变量: env
- 查看所有本地定义的环境变量: set
- 删除指定的环境变量: unset
- 只对当前shell(BASH)有效: export CLASS_PATH=./JAVA_HOME/lib:$JAVA_HOME/jre/lib,可以通过执行 source ~/.bashrc使当前用户有效或source vim /etc/profile对所有用户有效
- 常用的环境变量

```
PATH        决定了shell将到哪些目录中寻找命令或程序
HOME        当前用户主目录
HISTSIZE　  历史记录数
LOGNAME     当前用户的登录名
HOSTNAME　  主机的名称
SHELL 　　  当前用户Shell类型
```

### 运行程序

1. 命令行运行

```
 ./filename  
 如果在path设置了环境变量，无需./  如果关闭了命令行，则程序关闭
```

2. 后台运行

   ```
   nohup filename > output.txt 2>&1 &  
   表示在后台运行程序，标准输出和错误输出保存到文件
   要关闭程序，可通过命令 ps -ef |grep filename |awk '{print $2}'|xargs kill -9
   ```

3. 以服务方式运行

   ```
   /lib/systemd/system 目录下会创建一个XXX.service 的配置文件，里面定义了如何启动、如何关闭X86体系
   ```

![img](https://static001.geekbang.org/resource/image/ff/f0/ffb6847b94cb0fd086095ac263ac4ff0.jpg)

![img](https://static001.geekbang.org/resource/image/88/e5/8855bb645d8ecc35c80aa89cde5d16e5.jpg)

