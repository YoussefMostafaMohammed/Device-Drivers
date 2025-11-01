#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/proc_fs.h>  
#include <asm/errno.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)  
    #define HAVE_PROC_OPS  
#endif  

#define procfs_name "helloworld"  
#define DEVICE_NAME "character_device"
static unsigned long procfs_buffer_size = 0;  


static struct proc_dir_entry *our_proc_file;  
static void __exit my_module_exit(void);
static int __init my_module_init(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t*);
static ssize_t procfile_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t procfile_write(struct file *, const char __user *, size_t, loff_t *);

static int major;
enum {CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN};
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

#define BUFFER_LENGTH 80
static char msg[BUFFER_LENGTH+1];

#define PROCFS_MAX_SIZE 1024    
static char procfs_buffer[PROCFS_MAX_SIZE];  

static struct class* cls = NULL;
static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_file_fops = {  
    .proc_read = procfile_read,
    .proc_write = procfile_write,
};  
#else  
static const struct file_operations proc_file_fops = {
 
    .read = procfile_read,  
    .write = procfile_write,
};  
#endif  
  
static char *chardev_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;  // rw-rw-rw-
    return NULL;
}

static ssize_t procfile_read(struct file *filePointer, char __user *buffer, size_t buffer_length, loff_t *offset)  
{  
   int len = strlen(procfs_buffer);  // actual content length
    ssize_t ret = len;

    if (*offset >= len) {
        // user already read everything
        return 0;
    }

    if (copy_to_user(buffer, procfs_buffer, len)) {
        pr_info("copy_to_user failed\n");
        return -EFAULT;
    }

    *offset += len;
    pr_info("procfile read: %s\n", procfs_buffer);

    return ret;
}


static ssize_t procfile_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset)  
{  
    procfs_buffer_size = len;
 
    if (procfs_buffer_size >= PROCFS_MAX_SIZE)  
        procfs_buffer_size = PROCFS_MAX_SIZE - 1;
 
  
    if (copy_from_user(procfs_buffer, buffer, procfs_buffer_size))  
        return -EFAULT;  

 
    procfs_buffer[procfs_buffer_size] = '\0';  
    *offset += procfs_buffer_size;
 
    pr_info("procfile write %s\n", procfs_buffer);  
  
    return procfs_buffer_size;
}


static int __init my_module_init(void){
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Registering char device failed with %d\n", major);
        return major;
    }
    printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);
    
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)  
    cls = class_create(DEVICE_NAME);
#else  
    cls = class_create(THIS_MODULE, DEVICE_NAME);  
#endif
    cls->devnode = chardev_devnode;
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);      
    pr_info("Device created on /dev/%s\n", DEVICE_NAME);  
    
    our_proc_file = proc_create(procfs_name, 0666, NULL, &proc_file_fops);  
    if (our_proc_file == NULL) {  
        printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", procfs_name);  
        return -ENOMEM;  
    }  
    printk(KERN_INFO "/proc/%s created\n", procfs_name);
    return 0;
}

static void __exit my_module_exit(void){
    // printk(KERN_INFO "Goodbye from %s\n", my_name);
    device_destroy(cls, MKDEV(major, 0));
    class_unregister(cls);
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
    proc_remove(our_proc_file);
    return;
}

static int device_open(struct inode *inode, struct file *file){
    static int counter = 0; 
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
        return -EBUSY;
    sprintf(msg, "I already told you %d times Hello world!\n", counter++);
    return 0;
}

static int device_release(struct inode *inode, struct file *file){
    atomic_set(&already_open, CDEV_NOT_USED);
    return 0;
}

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t * offset){
    int bytes_read = 0;
    const char *msg_ptr = msg;
    if (!*(msg_ptr+(*offset))){
        *offset=0;
        return 0;
    }
    msg_ptr += *offset;
    while (length && *msg_ptr){ 
        put_user(*(msg_ptr++), buffer++);
        length--;
        bytes_read++;
    }
    offset += bytes_read;
    return bytes_read;
}

static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t * off){
    int bytes_written = 0;
    const char *msg_ptr = buff;
    if(len >= BUFFER_LENGTH){
        len = BUFFER_LENGTH-1;
     }
    if (copy_from_user(msg, buff, len))
        return -EFAULT;
    msg[len] = '\0';
    bytes_written = len;
    printk(KERN_INFO "device_write: received %zu bytes from user: \"%s\"\n", len, msg);
    return bytes_written;
}

module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");

