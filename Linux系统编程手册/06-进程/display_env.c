#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern char **environ;

/**
 *  打印参数和环境变量
 * cc display_env.c 
MacOS-----------------------------------------------------------------
argv[0] = ./a.out
argv[1] = hello
argv[2] = sky
TERM_PROGRAM=Apple_Terminal
TERM=xterm-256color
SHELL=/bin/bash
TMPDIR=/var/folders/3d/zndxynsd2777kl7gwlt6hnf80000gn/T/
Apple_PubSub_Socket_Render=/private/tmp/com.apple.launchd.YKCYtv51RZ/Render
TERM_PROGRAM_VERSION=404
OLDPWD=/Users/sky/work/practice/linux-note/05-深入探究文件IO
TERM_SESSION_ID=961D207E-21BD-422D-9DBD-5B457C5944E0
USER=sky
SSH_AUTH_SOCK=/private/tmp/com.apple.launchd.kOnWiaLE7S/Listeners
PATH=/Library/Frameworks/Python.framework/Versions/3.7/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:/Users/sky/Library/Android/sdk/tools:/Users/sky/Library/Android/sdk/platform-tools://Users/sky/work/software/apache-maven-3.5.4/bin://Users/sky/software/mongodb-osx-x86_64-4.0.6/bin
PWD=/Users/sky/work/practice/linux-note/06-进程
LANG=zh_CN.UTF-8
XPC_FLAGS=0x0
XPC_SERVICE_NAME=0
HOME=/Users/sky
SHLVL=1
LOGNAME=sky
_=./a.out

Ubuntu-----------------------------------------------------------------
./a.out 
argv[0] = ./a.out
SHELL=/bin/bash
SESSION_MANAGER=local/hefeng:@/tmp/.ICE-unix/2995,unix/hefeng:/tmp/.ICE-unix/2995
QT_ACCESSIBILITY=1
COLORTERM=truecolor
XDG_CONFIG_DIRS=/etc/xdg/xdg-ubuntu:/etc/xdg
XDG_MENU_PREFIX=gnome-
GNOME_DESKTOP_SESSION_ID=this-is-deprecated
LANGUAGE=zh_CN:zh
GNOME_SHELL_SESSION_MODE=ubuntu
SSH_AUTH_SOCK=/run/user/1000/keyring/ssh
XMODIFIERS=@im=ibus
DESKTOP_SESSION=ubuntu
SSH_AGENT_PID=2890
GTK_MODULES=gail:atk-bridge
PWD=/home/work1/workplace/github/linux-note/Linux系统编程手册/06-进程
LOGNAME=hefeng
XDG_SESSION_DESKTOP=ubuntu
XDG_SESSION_TYPE=x11
GPG_AGENT_INFO=/run/user/1000/gnupg/S.gpg-agent:0:1
XAUTHORITY=/run/user/1000/gdm/Xauthority
GJS_DEBUG_TOPICS=JS ERROR;JS LOG
WINDOWPATH=2
HOME=/home/hefeng
USERNAME=hefeng
IM_CONFIG_PHASE=1
LANG=zh_CN.UTF-8
LS_COLORS=rs=0:di=01;34:ln=01;36:mh=00:pi=40;33:so=01;35:do=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:mi=00:su=37;41:sg=30;43:ca=30;41:tw=30;42:ow=34;42:st=37;44:ex=01;32:*.tar=01;31:*.tgz=01;31:*.arc=01;31:*.arj=01;31:*.taz=01;31:*.lha=01;31:*.lz4=01;31:*.lzh=01;31:*.lzma=01;31:*.tlz=01;31:*.txz=01;31:*.tzo=01;31:*.t7z=01;31:*.zip=01;31:*.z=01;31:*.dz=01;31:*.gz=01;31:*.lrz=01;31:*.lz=01;31:*.lzo=01;31:*.xz=01;31:*.zst=01;31:*.tzst=01;31:*.bz2=01;31:*.bz=01;31:*.tbz=01;31:*.tbz2=01;31:*.tz=01;31:*.deb=01;31:*.rpm=01;31:*.jar=01;31:*.war=01;31:*.ear=01;31:*.sar=01;31:*.rar=01;31:*.alz=01;31:*.ace=01;31:*.zoo=01;31:*.cpio=01;31:*.7z=01;31:*.rz=01;31:*.cab=01;31:*.wim=01;31:*.swm=01;31:*.dwm=01;31:*.esd=01;31:*.jpg=01;35:*.jpeg=01;35:*.mjpg=01;35:*.mjpeg=01;35:*.gif=01;35:*.bmp=01;35:*.pbm=01;35:*.pgm=01;35:*.ppm=01;35:*.tga=01;35:*.xbm=01;35:*.xpm=01;35:*.tif=01;35:*.tiff=01;35:*.png=01;35:*.svg=01;35:*.svgz=01;35:*.mng=01;35:*.pcx=01;35:*.mov=01;35:*.mpg=01;35:*.mpeg=01;35:*.m2v=01;35:*.mkv=01;35:*.webm=01;35:*.ogm=01;35:*.mp4=01;35:*.m4v=01;35:*.mp4v=01;35:*.vob=01;35:*.qt=01;35:*.nuv=01;35:*.wmv=01;35:*.asf=01;35:*.rm=01;35:*.rmvb=01;35:*.flc=01;35:*.avi=01;35:*.fli=01;35:*.flv=01;35:*.gl=01;35:*.dl=01;35:*.xcf=01;35:*.xwd=01;35:*.yuv=01;35:*.cgm=01;35:*.emf=01;35:*.ogv=01;35:*.ogx=01;35:*.aac=00;36:*.au=00;36:*.flac=00;36:*.m4a=00;36:*.mid=00;36:*.midi=00;36:*.mka=00;36:*.mp3=00;36:*.mpc=00;36:*.ogg=00;36:*.ra=00;36:*.wav=00;36:*.oga=00;36:*.opus=00;36:*.spx=00;36:*.xspf=00;36:
XDG_CURRENT_DESKTOP=ubuntu:GNOME
VTE_VERSION=6003
GNOME_TERMINAL_SCREEN=/org/gnome/Terminal/screen/137da84d_850f_4d0b_b5b4_3c1d8c5f7eb6
INVOCATION_ID=7b7a2476a8e24962bcb9d4d7d05935ad
MANAGERPID=2637
GJS_DEBUG_OUTPUT=stderr
LESSCLOSE=/usr/bin/lesspipe %s %s
XDG_SESSION_CLASS=user
TERM=xterm-256color
LESSOPEN=| /usr/bin/lesspipe %s
USER=hefeng
GNOME_TERMINAL_SERVICE=:1.111
DISPLAY=:0
SHLVL=1
QT_IM_MODULE=ibus
LD_LIBRARY_PATH=:/opt/gcc-linaro-7.3.1-2018.05-i686_aarch64-elf/lib
XDG_RUNTIME_DIR=/run/user/1000
JOURNAL_STREAM=8:36710
XDG_DATA_DIRS=/usr/share/ubuntu:/usr/local/share/:/usr/share/
PATH=/home/hefeng/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/opt/gcc-linaro-7.3.1-2018.05-i686_aarch64-elf/bin:/opt/riscv-none-gcc/7.2.0-4-20180606-1631/bin:/opt/gcc-linaro-aarch64-linux-gnu-4.9-2014.09_linux/bin
GDMSESSION=ubuntu
DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus
OLDPWD=/home/work1/workplace/github/linux-note/Linux系统编程手册/05-深入探究文件IO
_=./a.out


 */
int
main(int argc, char *argv[])    
{
    for (int i=0; i<argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);
    
    for (char **p=environ; *p!=NULL; p++)
        puts(*p);

    exit(EXIT_SUCCESS);
}

