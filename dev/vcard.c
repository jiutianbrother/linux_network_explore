#include <linux/module.h>
#include <linux/netdevice.h>

#define TAG "VCARD: "

// 网卡名字
#define DEV_NAME "vcard"


// 网卡设备
static struct net_device *vcard_device = NULL;

// init()回调
static int vcard_init(struct net_device *dev)
{
    printk(TAG "in init: reg_state=%d\n", dev->reg_state);
    return 0;
}

// uninit()回调
static void vcard_uninit(struct net_device *dev)
{
    printk(TAG "in uninit: reg_state=%d\n", dev->reg_state);
}

// destructor()回调
void vcard_destructor(struct net_device *dev)
{
    printk(TAG "in destructor: reg_state=%d\n", dev->reg_state);
}

static struct net_device_ops vcard_device_ops = {
    .ndo_init = &vcard_init,
    .ndo_uninit = &vcard_uninit,
};

// 初始化网卡
static void vcard_setup(struct net_device *dev)
{
    dev->netdev_ops = &vcard_device_ops;
    dev->destructor = &vcard_destructor;

    printk(TAG "in setup: reg_state=%d\n", dev->reg_state);
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
    printk(TAG "after register_netdev: reg_state=%d\n", vcard_device->reg_state);
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

    ret = install_net_device();
    if (ret != 0) {
        return ret;
    }
    printk(TAG "vcard_init success exit\n");
    return 0;
}

static void __exit vcard_module_exit(void)
{
    uninstall_net_device();
}

module_init(vcard_module_init);
module_exit(vcard_module_exit);

MODULE_LICENSE("GPL");
