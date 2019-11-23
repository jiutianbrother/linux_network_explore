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

struct net_device *vcard_dev = NULL;

// 打开设备回调
static int vcard_open(struct net_device *dev)
{
    // 清除__LINK_STATE_XOFF标记，否则无法进行发送过程
    netif_start_queue(dev);
    return 0;
}

// 发送数据包回调
static netdev_tx_t vcard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    char data[6] = {'\0'};

    if (skb == NULL) {
        printk(TAG "%s: skb is null\n", __func__);
        return NETDEV_TX_OK;
    }

    if (skb->len != CONTENT_LEN) {
        printk(TAG "%s: wrong skb length(%d)\n", __func__, skb->len);
        goto done;
    }

    memcpy(data, skb->data, CONTENT_LEN);
    printk(TAG "%s: recv %s\n", __func__, data);

done:
    kfree_skb(skb);
    return NETDEV_TX_OK;
}

static struct net_device_ops vcard_device_ops = {
    .ndo_open = &vcard_open,
    .ndo_start_xmit = &vcard_start_xmit,
};

// 初始化网卡
static void vcard_setup(struct net_device *dev)
{
    // 必须要对netdev_ops进行赋值，否则在注册时会空指针异常
    dev->netdev_ops = &vcard_device_ops;

    // 指定tx_queue_len，表示我们使用流量控制机制发送
    dev->tx_queue_len = 5;
    // 正确的做法应该是实现ndo_change_mtu()回调进行MTU的配置，
    // 这里这么做仅仅是为了方便
    dev->mtu = 1500;
}

static int install_net_device(void)
{
    int ret;

    // 分配网卡对象
    vcard_dev = alloc_netdev(0, DEV_NAME, vcard_setup);
    if (!vcard_dev) {
        printk(TAG "alloc_netdev failed\n");
        return -ENOMEM;
    }

    // 将网卡注册到系统中
    ret = register_netdev(vcard_dev);
    if (ret) {
        printk(TAG "register_netdev failed\n");
        goto free;
    }
    return 0;

free:
    free_netdev(vcard_dev);
    vcard_dev = NULL;
    return ret;
}

static void uninstall_net_device(void)
{
    if (vcard_dev) {
        unregister_netdev(vcard_dev);
        free_netdev(vcard_dev);
        vcard_dev = NULL;
    }
}

static int __init vcard_module_init(void)
{
    int ret = 0;

    ret = install_net_device();
    if (ret != 0) {
        return ret;
    }
    return 0;
}

static void __exit vcard_module_exit(void)
{
    uninstall_net_device();
}

module_init(vcard_module_init);
module_exit(vcard_module_exit);

MODULE_LICENSE("GPL");
