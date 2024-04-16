#include <pwd.h>
#include <stdlib.h>

/**
 * 循环打印 /etc/passwd 中的 uname 和 uid
 * cc t_getpwent.c -o t_getpwent

./t_getpwent 
root         0
daemon       1
bin          2
sys          3
sync         4
games        5
man          6
lp           7
mail         8
news         9
uucp        10
proxy       13
www-data    33
backup      34
list        38
irc         39
gnats       41
nobody   65534
systemd-network   100
systemd-resolve   101
systemd-timesync   102
messagebus   103
syslog     104
_apt       105
tss        106
uuidd      107
tcpdump    108
avahi-autoipd   109
usbmux     110
rtkit      111
dnsmasq    112
cups-pk-helper   113
speech-dispatcher   114
avahi      115
kernoops   116
saned      117
nm-openvpn   118
hplip      119
whoopsie   120
colord     121
geoclue    122
pulse      123
gnome-initial-setup   124
gdm        125
sssd       126
hefeng    1000
systemd-coredump   999
fwupd-refresh   127
sshd       128

*/
int
main(int argc, char *argv[])
{
    struct passwd *pwd;

    while ((pwd = getpwent()) != NULL)
        printf("%-8s %5ld\n", pwd->pw_name, (long) pwd->pw_uid);
    endpwent();

    exit(EXIT_SUCCESS);
}