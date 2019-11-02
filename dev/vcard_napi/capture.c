#include <stdio.h>
#include <linux/if_ether.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>

// 根据网卡名字查询到网络设备的索引
static int get_if_index(const char *name)
{
    struct if_nameindex *ifni = if_nameindex();
    int i;

    for (i = 0; (ifni[i].if_index != 0 || ifni[i].if_name != NULL); ++i) {
        if (!strcmp(ifni[i].if_name, name)) {
            break;
        }
    }
    if (ifni[i].if_index != 0 || ifni[i].if_name != NULL) {
        return ifni[i].if_index;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // RAW类型的IP报文
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (fd < 0) {
        printf("socket fail\n");
        return -1;
    }

    int ifindex = get_if_index("vcard");
    if (ifindex <= 0) {
        printf("get_if_index fail\n");
        return -1;
    }

    // 绑定到指定的网络设备，这样只会接收到来自该数据的包
    struct sockaddr_ll addr = {
        .sll_family = AF_PACKET,
        .sll_protocol = htons(ETH_P_IP),
        .sll_ifindex = ifindex,
    };
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        printf("bind fail\n");
        return -1;
    }

    char buf[20];
    while (1) {
        int ret = recv(fd, buf, 20, MSG_TRUNC);
        if (ret < 0) {
            printf("recv fail\n");
            return -1;
        } else if (ret == 0) {
            continue;
        } else {
            buf[ret] = 0;
            printf("%s\n", buf);
            if (strcmp(buf, "ZZZZZ") == 0) {
                break;
            }
        }
    }
    return 0;
}
