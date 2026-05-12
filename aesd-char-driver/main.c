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
#include <linux/mutex.h>
#include "aesdchar.h"
#include "aesd-circular-buffer.h"
#include "aesd_ioctl.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Sara Edwards"); 
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

DEFINE_MUTEX(aesd_mutex);

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev* dev;

    PDEBUG("open");
         
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{

    PDEBUG("release");
    
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev* dev = filp->private_data;
    struct aesd_buffer_entry* retEntry;
    size_t retOffset;
    int bytesRem;
    int bytesNotCopied;
    
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);


    // get the semaphore before we write to the buffer
    if ((retval = mutex_lock_interruptible(&aesd_mutex)) != 0) return retval;

    if ((retEntry = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->buff),
            (size_t)(*f_pos), &retOffset)) == NULL) {
        mutex_unlock(&aesd_mutex);
        return 0;
    }
            
    bytesRem = retEntry->size - retOffset;
    bytesNotCopied = copy_to_user(buf, &(retEntry->buffptr[retOffset]), bytesRem);

    mutex_unlock(&aesd_mutex);
    
    // update f_pos
    *f_pos = *f_pos + bytesRem - bytesNotCopied;
    
    // return the number of bytes we read
    retval = bytesRem - bytesNotCopied;
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    int retval = -ENOMEM;
    char* writeBuff;
    const char* memToFree;
    struct aesd_dev* dev = filp->private_data;
    struct aesd_buffer_entry newEntry;
    int bytesNotCopied;
    
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    // copy the buffer into our holding buffer
    bytesNotCopied = copy_from_user(&(dev->holdingBuff[dev->holdingBuffSize]), buf, count);
    
    dev->holdingBuffSize += count - bytesNotCopied;
    
    // if this write completes a command, do the write
    if (dev->holdingBuff[dev->holdingBuffSize - 1] == '\n') {
    
        writeBuff = kmalloc(dev->holdingBuffSize, GFP_KERNEL);

        strncpy(writeBuff, dev->holdingBuff, dev->holdingBuffSize);
        newEntry.buffptr = writeBuff;
        newEntry.size = dev->holdingBuffSize;
        *f_pos = *f_pos + dev->holdingBuffSize;

        // get the semaphore before we write to the buffer
        if ((retval = mutex_lock_interruptible(&aesd_mutex)) == -EINTR) {
            kfree(writeBuff);
            return (ssize_t)(-ERESTARTSYS);
        }
    
        memToFree = aesd_circular_buffer_add_entry(&(dev->buff), &newEntry);
        
        mutex_unlock(&aesd_mutex);
        
        // free memory from overwritten entry
        kfree(memToFree);
        
        // reset the holding buffer
        dev->holdingBuffSize = 0;
    }

    return (ssize_t)(count - bytesNotCopied);
}

loff_t aesd_llseek(struct file *filp, loff_t offset, int whence){
    struct aesd_buffer_entry* entry;
    size_t retByteOffset;
    struct aesd_dev* dev = filp->private_data;
    loff_t newOffset = offset;
    int errval = 0;
    size_t retval = 0;
    
    PDEBUG("seek %lld bytes from %d (0=SET,1=CUR,2=END)", offset, whence);

    if ((retval = mutex_lock_interruptible(&aesd_mutex)) != 0) return retval;
    
    // if offset is more than we have stored
    if ((entry = aesd_circular_buffer_find_entry_offset_for_fpos
                   (&(dev->buff), offset, &retByteOffset)) == NULL) {
        errval = -EINVAL;
    }
    else {
        switch (whence) {
            case SEEK_SET:

                // set the position to the input offset
                newOffset = offset;
                break;
            
            case SEEK_CUR:
            
                //set the position to the current position + the input offset
                newOffset = filp->f_pos + offset;
            
                break;
            
            case SEEK_END:

                //set the position to the current position + the input offset
                newOffset = filp->f_pos - offset;
            
                break;

            default:  
                errval = -EINVAL;
                break;
        }

        // error if the new position is outside the file
        if (((entry = aesd_circular_buffer_find_entry_offset_for_fpos
                       (&(dev->buff), newOffset, &retByteOffset)) == NULL) || 
                       (newOffset < 0)) errval = -EINVAL;
        else filp->f_pos = newOffset;
    }
    
    mutex_unlock(&aesd_mutex);
            
    return (errval == 0 ? newOffset : errval);
}

int aesd_adjust_file_offset(struct file* filp, unsigned int write_cmd, unsigned int write_cmd_offset) {
    struct aesd_dev* dev = filp->private_data;
    struct aesd_buffer_entry* entryptr;
    int index;
    int new_f_pos = 0;

    // get the mutex before we access the buffer
    if ((retval = mutex_lock_interruptible(&aesd_mutex)) != 0) return retval;
    
    // verify the write_cmd is in range
    if (write_cmd >= AESD_CIRCULAR_BUFFER_NUMBERUSED(&(dev->buff))) {
        mutex_unlock(&aesd_mutex);
        return -EINVAL;
    }

    // add up the number of bytes in each used entry before the one that contains the offset
    AESD_CIRCULAR_BUFFER_FOREACH(entryptr,&(dev->buff),index) {
        if (index < write_cmd) {
            new_f_pos += entryptr->size;
        }
    }

    // verify the write_cmd_offset is in range
    if (write_cmd_offset >= dev->buff.entry[write_cmd].size) {
        mutex_unlock(&aesd_mutex);
        return -EINVAL;
    }
    
    // release the mutex
    mutex_unlock(&aesd_mutex);
    
    // add in the offset within this entry
    new_f_pos += write_cmd_offset;
    
    // record the new position
    filp->f_pos = new_f_pos;
    
    return 0;  // success
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    
    struct aesd_seekto seekto;
    long retval = 0;

    switch (cmd) {
    
        case AESDCHAR_IOCSEEKTO:
            if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto)) != 0) {
                retval = EFAULT;
            }
            else {
                retval = aesd_adjust_file_offset(filp, seekto.write_cmd, seekto.write_cmd_offset);
            }
            break;
            
        default:  
            retval = -EINVAL;
            break;
    }
    
    return retval;
}

struct file_operations aesd_fops = {
    .owner =          THIS_MODULE,
    .read =           aesd_read,
    .write =          aesd_write,
    .open =           aesd_open,
    .release =        aesd_release,
    .llseek =         aesd_llseek,
    .unlocked_ioctl = aesd_ioctl,
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
    
    // mutex is initialized upon static declaration; nothing to do here

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    struct aesd_buffer_entry* entryptr;
    int index;

    // kfree(dev); if using kalloc

    // free the memory used in the circular buffer
    AESD_CIRCULAR_BUFFER_FOREACH(entryptr, &(aesd_device.buff), index) {
        kfree(entryptr->buffptr);
    }
    
    cdev_del(&aesd_device.cdev);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
