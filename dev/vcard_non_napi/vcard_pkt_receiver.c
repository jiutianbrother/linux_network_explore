#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/net_namespace.h>

#define TAG "VCARD_RCV: "

#define DEV_NAME "vcard"

#define CONTENT_LEN 5

static int vcard_pkt_receiver_rcv(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt, struct net_device *orig_dev)
{
    char content[CONTENT_LEN+1];
    int ret;

    if (skb->len - skb->data_len != CONTENT_LEN) {
        printk(TAG "vcard_pkt_receiver_rcv: invalid skb len\n");
        ret = NET_RX_DROP;
        goto out;
    }
    
    memset(content, 0, CONTENT_LEN+1);
    memcpy(content, skb->data, CONTENT_LEN);
    printk(TAG "%s\n", content);
    ret = NET_RX_SUCCESS;
out:
    kfree_skb(skb);
    return ret;
}

static struct packet_type vcard_pkt_type = {
	.type = __constant_htons(ETH_P_IP),
	.func = vcard_pkt_receiver_rcv,
	.dev = NULL,
};

static int __init vcard_pkt_receiver_init(void)
{
    struct net_device *dev = dev_get_by_name(&init_net, DEV_NAME);
    if (dev == NULL) {
        printk(TAG "vcard_pkt_receiver_init: get device failed\n");
        return -1;
    }
    vcard_pkt_type.dev = dev;
    dev_add_pack(&vcard_pkt_type);
    return 0;
}

static void __exit vcard_pkt_receiver_exit(void)
{
    // dev_get_by_name()会持有dev的一个引用计数，模块卸载时应该释放.
    // 实际上这是个错误的示范，正确的做法应该是监听网络设备注册状态
    // 的变化，当收到去注册事件时应该主动指定dev_put()释放引用计数
    if (vcard_pkt_type.dev != NULL)
        dev_put(vcard_pkt_type.dev);
    dev_remove_pack(&vcard_pkt_type);
}

module_init(vcard_pkt_receiver_init);
module_exit(vcard_pkt_receiver_exit);

MODULE_LICENSE("GPL");