#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/string.h> 
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/completion.h>
#include <linux/dmaengine.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h> 
#include <linux/cdev.h> 
#include <linux/dmaengine.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/jiffies.h>



#define BUFFER_SIZE 1024
#define MEM_CPY_NO_DMA 0
#define MEM_CPY_DMA    1

struct dma_msg_struct{
	u32 buf[1024];
	int len;
};




char k_buf[1024];


struct hello_struct{
	/*创建设备*/
	dev_t dev;
	struct cdev cdev;
	struct class *class;
	struct dma_chan * channel;
	/*完成回调*/
	
	
};

struct completion transfer_ok;


static struct hello_struct *h_msg;

dma_addr_t src_dma_addr;
dma_addr_t dst_dma_addr;
dma_addr_t * vtx;
dma_addr_t * vrx;








static void my_dma_callback(void *param){
//	complete(&transfer_ok);
	printk("callback is ok\n");
	printk("vrx = %s\n",vrx);

	return ;
}




ssize_t hello_drv_read(struct file * file, char __user * buf, size_t count, loff_t * offset){
	int ret;
	printk(" %s %s %d \n",__FILE__,__FUNCTION__,__LINE__);
	ret=copy_to_user(buf,vrx, count);
	printk("ret = %d\n",ret);
	return ret;

}


int hello_drv_open(struct inode *inode, struct file *file){
	printk(" %s %s %d \n",__FILE__,__FUNCTION__,__LINE__);

	
	
	return 0;
}


int hello_drv_close(struct inode *inode, struct file *file){
	printk(" %s %s %d \n",__FILE__,__FUNCTION__,__LINE__);
	
	
	return 0;
}


long hello_dma_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	int ret;
	struct dma_msg_struct *dma_msg;
	struct dma_device *dma_dev;
	dma_msg=kzalloc(sizeof(struct dma_msg_struct) ,GFP_KERNEL);

	
	if (copy_from_user(dma_msg, (struct dma_msg_struct __user *)arg, sizeof(struct dma_msg_struct)))
	{
		printk(KERN_ERR " copy_from_user() fail.\n");
		return -EINVAL;
	}

	switch (cmd){
		case MEM_CPY_NO_DMA:{
			
			/*不使用dma得话 直接进行memcpy*/
			memcpy(vrx,dma_msg->buf,dma_msg->len);
			printk("memcpy is success !\n");
			break;
		}
		case MEM_CPY_DMA:{





			
			
			//wait_for_completion(&transfer_ok);
			//wait_for_completion_interruptible(&transfer_ok);

			printk("dma transfer is success !\n");
			break;
		
		}
	}
return 0;
}

static struct file_operations hello_drv={
	.owner=THIS_MODULE,
	.open=hello_drv_open,
	.read=hello_drv_read,
	.release=hello_drv_close,
	.unlocked_ioctl = hello_dma_ioctl,

};



static int __init hello_drv_init(void){
	printk(" %s %s %d \n",__FILE__,__FUNCTION__,__LINE__);
	// pr_info("hello world\n");

	h_msg=kzalloc(sizeof(struct hello_struct), GFP_KERNEL);

	alloc_chrdev_region(&h_msg->dev, 0, 1, "hello");
	cdev_init(&h_msg->cdev, &hello_drv);
	cdev_add(&h_msg->cdev, h_msg->dev, 1);
	h_msg->class=class_create(THIS_MODULE, "hello_class");
	device_create(h_msg->class, NULL, h_msg->dev, NULL, "hello_0");
	
	vtx=dma_alloc_coherent(NULL, BUFFER_SIZE, &src_dma_addr, GFP_KERNEL);
	vrx=dma_alloc_coherent(NULL, BUFFER_SIZE, &dst_dma_addr, GFP_KERNEL);

	

	
	printk("虚拟发送地址为:0x%x\n",vtx);
	printk("虚拟接受地址为:0x%x\n",vrx);
	printk("物理发送地址为:0x%x\n",src_dma_addr);
	printk("物理接受地址为:0x%x\n",dst_dma_addr);

	/*设置dma的channel的类型*/
	dma_cap_mask_t mask;
	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);
	h_msg->channel= dma_request_channel(mask, NULL, NULL);
	if(!(h_msg->channel)){
		dma_free_coherent(NULL, BUFFER_SIZE, vrx,dst_dma_addr);
		dma_free_coherent(NULL, BUFFER_SIZE, vtx,src_dma_addr);
		printk("channel error\n");
		return -1;
	}
	printk(KERN_INFO "dma channel id = %d\n",h_msg->channel->chan_id);
	//struct dma_slave_config my_dma_cfg={0};
	//my_dma_cfg.direction=DMA_MEM_TO_MEM;
	//my_dma_cfg.dst_addr_width=DMA_SLAVE_BUSWIDTH_32_BYTES;
	//dmaengine_slave_config(h_msg->channel, &my_dma_cfg);
	
	
	
	//memcpy(vtx,dma_msg->buf,dma_msg->len);

	/*获取事务描述符*/
	struct dma_async_tx_descriptor *tx=NULL;
	
	memset(vrx,0xAA,BUFFER_SIZE);
	memset(vtx,0x56,BUFFER_SIZE);
	
	tx=h_msg->channel->device->device_prep_dma_memcpy(h_msg->channel,dst_dma_addr,src_dma_addr,BUFFER_SIZE,0);
	if (!tx){
		printk(KERN_INFO "Failed to prepare DMA memcpy");
	
		return -1;
	}
	/*回调处理：传输完成后调用*/
	//init_completion(&transfer_ok);
	tx->callback=my_dma_callback;
	tx->callback_param = NULL;

	/*提交事务*/
	dma_cookie_t cookie=dmaengine_submit(tx);
	if(dma_submit_error(cookie)){
		printk("dma 提交事务失败\n");
		return -1;
	}

	/*发起传输*/
	dma_async_issue_pending(h_msg->channel);

	/* 等待传输完成 */
	enum dma_status dma_status = DMA_ERROR;
	struct dma_tx_state tx_state = {0};
	int cnt=0;
	/*判断dma 的status 查看是否完成*/
	 while(DMA_COMPLETE!= dma_status)
	{
		dma_status = h_msg->channel->device->device_tx_status(h_msg->channel, cookie, &tx_state);
		printk(KERN_INFO"dma_status = %d\n", dma_status);
		schedule();
		cnt++;
		if(cnt==10)
			break;
	}
	 
	printk(KERN_INFO"dma_status = %d\n", dma_status);	


	

	return 0;

}







static void __exit hello_drv_exit(void){
	printk(" %s %s %d \n",__FILE__,__FUNCTION__,__LINE__);
	// pr_info("bye world\n");
	dma_release_channel(h_msg->channel);
	dma_free_coherent(NULL, BUFFER_SIZE, vrx,dst_dma_addr);
	dma_free_coherent(NULL, BUFFER_SIZE, vtx,src_dma_addr);

	device_destroy(h_msg->class, h_msg->dev);
	class_destroy(h_msg->class);
	cdev_del(&h_msg->cdev);

	

}



module_init(hello_drv_init);
module_exit(hello_drv_exit);
MODULE_LICENSE("GPL");







