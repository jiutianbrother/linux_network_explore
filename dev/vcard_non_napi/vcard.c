#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/timer.h>
#include <linux/if_ether.h>
#include <linux/skbuff.h>

#define TAG "VCARD: "

// 网卡名字
#define DEV_NAME "vcard"

// 生成的数据包携带5个字节数据，"AAAAA"~"ZZZZZ"
#define CONTENT_LEN 5
static int loop = 26;
static char content = 'A';

// 网卡设备
static struct net_device *vcard_device = NULL;

// 模拟数据包到来的定时器
static struct timer_list pkt_kick_timer;
static void restart_rx_timer(void);

// 定时器处理函数
static void populate_pkt(unsigned long data)
{
    struct sk_buff *skb = NULL;

    if (loop) {
        skb = dev_alloc_skb(CONTENT_LEN);
        if (skb == NULL)
            goto start_timer;
        
        skb->dev = vcard_device;
        // 自定义的数据包没有mac头部，所以data指针不需要做调整
        skb_reset_mac_header(skb);
        skb_put(skb, CONTENT_LEN);
        memset(skb->data, content, CONTENT_LEN);
        skb->protocol = __constant_htons(ETH_P_IP); // 没关系，随便写一个就好
        skb->pkt_type = PACKET_HOST;
        netif_rx(skb);
        --loop;
        content += 1;
    } else {
        goto out;
    }
start_timer:
    restart_rx_timer();
out:
    return;
}

// 启动定时器，超时时间为1s
static void restart_rx_timer(void)
{
    init_timer(&pkt_kick_timer);
    pkt_kick_timer.expires = jiffies + HZ;
    pkt_kick_timer.function = populate_pkt;
    add_timer(&pkt_kick_timer);
}

static struct net_device_ops vcard_device_ops = {
};

// 初始化网卡
static void vcard_setup(struct net_device *dev)
{
    // 必须要对netdev_ops进行赋值，否则在注册时会空指针异常，尽管我们不关注它
    dev->netdev_ops = &vcard_device_ops;
}

static int install_net_device(void)
{
    int ret;

    // 分配网卡对象
    vcard_device = alloc_netdev(0, DEV_NAME, vcard_setup);
    if (!vcard_device) {
        printk(TAG "alloc_netdev failed\n");
        return -ENOMEM;
    }
    
    // 将网卡注册到系统中
    ret = register_netdev(vcard_device);
    if (ret) {
        printk(TAG "register_netdev failed\n");
        goto free;
    }
    return 0;

free:
    free_netdev(vcard_device);
    vcard_device = NULL;
    return ret;
}

static void uninstall_net_device(void)
{
    if (vcard_device) {
        unregister_netdev(vcard_device);
        free_netdev(vcard_device);
        vcard_device = NULL;
    }
}

static int __init vcard_module_init(void)
{
    int ret = 0;

    printk(TAG "vcard_init enter\n");
    ret = install_net_device();
    if (ret != 0) {
        return ret;
    }
    
    restart_rx_timer();
    printk(TAG "vcard_init success exit\n");
    return 0;
}

static void __exit vcard_module_exit(void)
{
    del_timer(&pkt_kick_timer);
    uninstall_net_device();
}

module_init(vcard_module_init);
module_exit(vcard_module_exit);

MODULE_LICENSE("GPL");
