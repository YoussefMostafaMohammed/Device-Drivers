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
#include <linux/seq_file.h>
#include <asm/errno.h>
#include <linux/kprobes.h>
#include <linux/cred.h>
#include <linux/wait.h>
#include <linux/sched/signal.h> /* signal_pending() */

static DECLARE_WAIT_QUEUE_HEAD(device_waitq);

static struct kprobe kp;

static uid_t target_uid = 1000; // Replace with the user ID you want to monitor

static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    kuid_t kuid = current_uid(); // get current process UID in kernel format

    if (__kuid_val(kuid) == target_uid) {
        printk(KERN_INFO "[kprobe] User %d executed openat: PID=%d, Comm=%s\n",
               __kuid_val(kuid),
               current->pid,
               current->comm);
    }

    return 0;
}
static void setup_kprobe(void)
{
    kp.symbol_name = "__x64_sys_openat";
    kp.pre_handler = handler_pre;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)  
    #define HAVE_PROC_OPS  
#endif  

#define procfs_name "seqfile"  
#define DEVICE_NAME "character_device_seq_file"
#define MAX_LINES 5
#define BUFFER_LENGTH 80
#define PROCFS_MAX_SIZE 1024

struct ioctl_arg {
 
    unsigned int val;  
};  
  
/* Documentation/userspace-api/ioctl/ioctl-number.rst */
 
#define IOC_MAGIC '\x66'  
#define IOCTL_VALSET _IOW(IOC_MAGIC, 0, struct ioctl_arg)
#define IOCTL_VALGET _IOR(IOC_MAGIC, 1, struct ioctl_arg)  
#define IOCTL_VALGET_NUM _IOR(IOC_MAGIC, 2, int)
#define IOCTL_VALSET_NUM _IOW(IOC_MAGIC, 3, int)  
#define IOCTL_VAL_MAXNR 3
#define DRIVER_NAME "character_device_seq_file"  
 
static unsigned int num_of_dev = 1;  
static struct cdev test_ioctl_cdev;
 
static int ioctl_num = 0;  
  
struct test_ioctl_data {
    rwlock_t lock;
    unsigned char val;      // For IOCTL read/write
    char buffer[100];       // For normal read/write
    size_t size;
};
  

static unsigned long procfs_buffer_size = 0;  
static struct proc_dir_entry *our_proc_file;  
static void __exit my_module_exit(void);
static int __init my_module_init(void);

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);   
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t*);
static long test_ioctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);  
static int my_open(struct inode *inode, struct file *file);
static void *my_seq_start(struct seq_file *s, loff_t *pos);
static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos);
static void my_seq_stop(struct seq_file *s, void *v);
static int my_seq_show(struct seq_file *s, void *v);

static int major;
enum {CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN};

static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static char msg[BUFFER_LENGTH+1];
static char procfs_buffer[PROCFS_MAX_SIZE];  

static struct class* cls = NULL;

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = test_ioctl_ioctl,  
};

#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_file_fops = {  
    .proc_open = my_open,
    .proc_read = seq_read,  
    .proc_lseek = seq_lseek,  
    .proc_release = seq_release,
};

#else  
static const struct file_operations proc_file_fops = {
    .open = my_open,
    .read = seq_read,  
    .llseek = seq_lseek,  
    .release = seq_release, 
};  
#endif  
  
static struct seq_operations my_seq_ops = {  
    .start = my_seq_start,
    .next = my_seq_next,  
    .stop = my_seq_stop,  
    .show = my_seq_show,
};  

static int speed_limit = 80;

ssize_t my_show(struct device *dev, struct device_attribute *attr,  char *buf){
    return sprintf(buf, "%d\n", speed_limit);  
}

static ssize_t my_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    sscanf(buf, "%d", &speed_limit);
    return count;
}

static struct device_attribute my_speedlimit= __ATTR(speed_limit, 0660, my_show, my_store);  

static void *my_seq_start(struct seq_file *s, loff_t *pos) {
    static unsigned long counter;
    if (*pos >= MAX_LINES)
        return NULL;
    counter = *pos;
    return &counter;
}

static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos) {

    unsigned long *tmp_v = v;
    (*tmp_v)++;
    (*pos)++;
    return (*pos >= MAX_LINES) ? NULL : tmp_v;
}

static void my_seq_stop(struct seq_file *s, void *v)
{
    /* nothing to do, we use a static value in start() */  
}
 
static int my_seq_show(struct seq_file *s, void *v)  
{
 
    loff_t *spos = (loff_t *)v;  
  
    seq_printf(s, "%Ld\n", *spos);
 
    return 0;  
}

static int my_open(struct inode *inode, struct file *file)  
{  
    return seq_open(file, &my_seq_ops);
};  

static char *chardev_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;  // rw-rw-rw-
    return NULL;
}

static int __init my_module_init(void){
    int ret;

    setup_kprobe();
    ret = register_kprobe(&kp);
    if (ret < 0) {
        printk(KERN_ERR "register_kprobe failed: %d\n", ret);
    } else {
        printk(KERN_INFO "Kprobe registered on openat\n");
    }
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
    struct device *dev =device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);      
    pr_info("Device created on /dev/%s\n", DEVICE_NAME);  
    
    our_proc_file = proc_create(procfs_name, 0666, NULL, &proc_file_fops);  
    if (our_proc_file == NULL) {  
        printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", procfs_name);  
        return -ENOMEM;  
    }  

    device_create_file(dev, &my_speedlimit); 

    printk(KERN_INFO "/proc/%s created\n", procfs_name);
    return 0;
}

static void __exit my_module_exit(void){
    unregister_kprobe(&kp);
    printk(KERN_INFO "Kprobe unregistered\n");
    device_destroy(cls, MKDEV(major, 0));
    class_unregister(cls);
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
    proc_remove(our_proc_file);
    return;
}
static long test_ioctl_ioctl(struct file *filp, unsigned int cmd,
 
                             unsigned long arg)  
{  

    struct test_ioctl_data *ioctl_data = filp->private_data;
    
    int retval = 0;  
    unsigned char val;  
    struct ioctl_arg data;
 
    memset(&data, 0, sizeof(data));  
  
    switch (cmd) {  
        case IOCTL_VALSET:
            if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
                retval = -EFAULT;  
                goto done;  
            }  
            pr_alert("IOCTL set val:%x .\n", data.val);  
            write_lock(&ioctl_data->lock);
            ioctl_data->val = data.val;  
            write_unlock(&ioctl_data->lock);
            break;  

        case IOCTL_VALGET:  
            read_lock(&ioctl_data->lock);
            val = ioctl_data->val;  
            read_unlock(&ioctl_data->lock);
            data.val = val;  
            if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
                retval = -EFAULT;  
                goto done;  
            }  
            break;  

        
        case IOCTL_VALGET_NUM:  
            retval = __put_user(ioctl_num,(int __user *)arg);        
            break;  
        
        case IOCTL_VALSET_NUM:  
            ioctl_num = arg;  
            break;  
 
        default:  
            retval = -ENOTTY;  
    }  
done:  
    return retval;
 
}  

static int device_open(struct inode *inode, struct file *file)
{
    struct test_ioctl_data *ioctl_data;
    static int counter = 0;

    /* Try to acquire the device (non-blocking check) */
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN) == CDEV_NOT_USED) {
        /* acquired successfully */
        goto setup;
    }

    /* Device is busy */
    if (file->f_flags & O_NONBLOCK)
        return -EAGAIN;

    /* Block until the device becomes available or we get a signal */
    while (1) {
        int ret;

        /* Wait until already_open becomes 0 */
        ret = wait_event_interruptible(device_waitq,
                                       atomic_read(&already_open) == CDEV_NOT_USED);
        if (ret == -ERESTARTSYS || signal_pending(current))
            return -EINTR; /* interrupted by signal */

        /* Try to acquire again */
        if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN) == CDEV_NOT_USED)
            break; /* got it, proceed */
        /* otherwise loop and wait again */
    }

setup:
    ioctl_data = kmalloc(sizeof(struct test_ioctl_data), GFP_KERNEL);
    if (ioctl_data == NULL) {
        /* release the claim if allocation fails */
        atomic_set(&already_open, CDEV_NOT_USED);
        wake_up_interruptible(&device_waitq);
        return -ENOMEM;
    }

    rwlock_init(&ioctl_data->lock);
    ioctl_data->val = 0xFF;
    file->private_data = ioctl_data;
    sprintf(msg, "I already told you %d times Hello world!\n", counter++);
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    /* free per-file private data */
    if (file->private_data) {
        kfree(file->private_data);
        file->private_data = NULL;
    }

    /* mark device as free and wake up waiters */
    atomic_set(&already_open, CDEV_NOT_USED);
    wake_up_interruptible(&device_waitq);

    return 0;
}

static ssize_t device_read(struct file *filp, char __user *buffer,
                           size_t length, loff_t *offset)
{
    struct test_ioctl_data *data = filp->private_data;
    ssize_t ret = 0;

    if (*offset >= data->size)
        return 0;

    if (length > data->size - *offset)
        length = data->size - *offset;

    read_lock(&data->lock);
    if (copy_to_user(buffer, &data->buffer[*offset], length))
        ret = -EFAULT;
    else {
        *offset += length;
        ret = length;
    }
    read_unlock(&data->lock);

    return ret;
}


static ssize_t device_write(struct file *filp, const char __user *buff,
                            size_t len, loff_t *offset)
{
    struct test_ioctl_data *data = filp->private_data;

    if (len >= sizeof(data->buffer))
        len = sizeof(data->buffer) - 1;

    write_lock(&data->lock);
    if (copy_from_user(data->buffer, buff, len)) {
        write_unlock(&data->lock);
        return -EFAULT;
    }
    data->buffer[len] = '\0';
    data->size = len;
    write_unlock(&data->lock);

    return len;
}

module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");