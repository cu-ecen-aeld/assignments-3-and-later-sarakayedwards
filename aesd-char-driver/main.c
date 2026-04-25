/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/string.h> 
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Sara Edwards"); 
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
         
    struct aesd_dev* dev;
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    
    aesd_dev* dev = filp->private_data;
    
    // free the memory used in the circular buffer
    for (i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
        kfree(dev->buff.entry[i].buffptr);
    }
    
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev* dev = filp->private_data;
    struct aesd_buffer_entry* retEntry;
    size_t retOffset;
    
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    retEntry = aesd_circular_buffer_find_entry_offset_for_fpos(dev->buff,
            (size_t)(*f_pos), size_t *retOffset );

    bytesRem = dev->buff.entry[retEntry].size - retOffset;
    retVal = copy_to_user(buf, &(dev->buff.entry[retEntry].buffptr[retOffset]), bytesRem);
     
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    char* writeBuff;
    char* memToFree;
    struct aesd_dev* dev = filp->private_data;
    
    
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    // copy the buffer into our holding buffer
    if (retval = copy_from_user(&(dev->holdingBuff[dev->holdingBuffSize]), buf, count) != 0) {
        return retval;
    }
    
    dev->holdingBuffSize += count;
    
    // if this write completes a command, do the write
    if (dev->holdingBuff[dev->holdingBuffSize - 1] == '\n') {
    
        writeBuff = kmalloc(dev->holdingBuffSize, GFP_KERNEL);

        struct aesd_buffer_entry newEntry;
        strncpy(writeBuff, dev->holdingBuff, dev->holdingBuffSize);
        newEntry.buffPtr = writeBuff;
        newEntry.size = dev->holdingBuffSize;

        // get the semaphore before we write to the buffer
        pthread_mutex_lock(&(dev->lock));
    
        memToFree = aesd_circular_buffer_add_entry(&(dev->buff), &newEntry);
        
        pthread_mutex_unlock(&(dev->lock));
        
        // free memory from overwritten entry
        kfree(memToFree);
        
        // reset the holding buffer
        dev->holdingBuffSize = 0;
    }

    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}

int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    
    //struct aesd_dev* aesd_device;
    // aesd_device = kalloc(sizeof(struct aesd_dev));
    
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    aesd_device.holdingBuffSize = 0; 
    aesd_circular_buffer_init(&(aesd_device.buff));
    
    pthread_mutextattr_init(&(aesd_device.lockAttr);
    pthread_mutex_init(&(aesd_device.lock), &(aesd_device.lockAttr));

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    // kfree(dev); if using kalloc

    cdev_del(&aesd_device.cdev);

    pthread_mutextattr_destroy(&(aesd_device.lockAttr);
    pthread_mutex_destroy(&(aesd_device.lock));
    
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
