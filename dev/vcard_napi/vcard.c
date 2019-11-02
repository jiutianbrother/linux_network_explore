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

// 网卡设备,NAPI模式必须提供一个napi_struct
struct vcard_net_device {
    struct net_device *dev;
    struct napi_struct napi;
};
struct vcard_net_device *vcard_dev = NULL;

// 模拟数据包到来的定时器
static struct timer_list pkt_kick_timer;
static void restart_rx_timer(void);

// 定时器处理函数
static struct sk_buff* populate_pkt(void)
{
    struct sk_buff *skb = NULL;

    if (loop) {
        skb = dev_alloc_skb(CONTENT_LEN);
        if (skb == NULL)
            goto out;

        skb->dev = vcard_dev->dev;
        // 自定义的数据包没有mac头部，所以data指针不需要做调整
        skb_reset_mac_header(skb);
        skb_put(skb, CONTENT_LEN);
        memset(skb->data, content, CONTENT_LEN);
        skb->protocol = __constant_htons(ETH_P_IP); // 没关系，随便写一个就好
        skb->pkt_type = PACKET_HOST;
        --loop;
        content += 1;
    }
out:
    return skb;
}

// 定时器处理函数
static void resume_napi(unsigned long data)
{
    // 由于是虚拟设备，也不涉及关中断(定时器自己只启动一次)，
    // 直接尝试激活软中断即可
    if (likely(napi_schedule_prep(&vcard_dev->napi))) {
        __napi_schedule(&vcard_dev->napi);
    }
}

// 启动定时器，超时时间为1s
static void restart_rx_timer(void)
{
    init_timer(&pkt_kick_timer);
    pkt_kick_timer.expires = jiffies + 30 * HZ;
    pkt_kick_timer.function = resume_napi;
    add_timer(&pkt_kick_timer);
}

// init()回调
static int vcard_init(struct net_device *dev)
{
    struct vcard_net_device *vdev = netdev_priv(dev);
    // 因为在netif_napi_add()会设置NAPI_STATE_SCHED标记，
    // 所以这里必须将该标记清除，否则napi_schedule_prep()
    // 会返回false，将无法启动接收软中断
    clear_bit(NAPI_STATE_SCHED, &vdev->napi.state);
    return 0;
}

static struct net_device_ops vcard_device_ops = {
    .ndo_init = &vcard_init,
};

// 初始化网卡
static void vcard_setup(struct net_device *dev)
{
    // 必须要对netdev_ops进行赋值，否则在注册时会空指针异常，尽管我们不关注它
    dev->netdev_ops = &vcard_device_ops;
}

// 非NAPI接收模式下的poll()回调实现
int vcard_poll(struct napi_struct *napi, int budget)
{
    struct vcard_net_device *vdev = container_of(napi, struct vcard_net_device, napi);
    struct sk_buff *skb;
    int rcv = 0;

    while (rcv < budget && (skb = populate_pkt()) != NULL) {
        ++rcv;
        netif_receive_skb(skb);
    }
    // 如果数据已经全部接收完毕，那么驱动需要负责将调度停止,
    // 这里要特别注意，必须是小于，不能是小于等于，因为等于
    // 表示刚好接收了指定配额个数据包，这个返回值对于NAPI接收
    // 框架是有特殊意义的
    if (rcv < budget)
        napi_complete(&vdev->napi);

    // 返回实际读取的数据包个数
    printk(TAG "vcard_poll ret=%d\n", rcv);
    return rcv;
}

static int install_net_device(void)
{
    int ret;
    struct net_device *dev;

    // 分配网卡对象
    dev = alloc_netdev(sizeof(struct vcard_net_device), DEV_NAME, vcard_setup);
    if (!dev) {
        printk(TAG "alloc_netdev failed\n");
        return -ENOMEM;
    }

    vcard_dev = netdev_priv(dev);
    vcard_dev->dev = dev;

    netif_napi_add(dev, &vcard_dev->napi, vcard_poll, 5);

    // 将网卡注册到系统中
    ret = register_netdev(dev);
    if (ret) {
        printk(TAG "register_netdev failed\n");
        goto free;
    }
    return 0;

free:
    free_netdev(dev);
    vcard_dev = NULL;
    return ret;
}

static void uninstall_net_device(void)
{
    if (vcard_dev) {
        unregister_netdev(vcard_dev->dev);
        free_netdev(vcard_dev->dev);
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

    restart_rx_timer();
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
