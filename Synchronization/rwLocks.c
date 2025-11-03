#include <linux/atomic.h>
#include <linux/module.h>
#include <linux/rwlock.h>
#include <linux/printk.h>

static DEFINE_RWLOCK(myrwlock);

static void my_read_lock(void){

    unsigned long flags;
    read_lock_irqsave(&myrwlock,flags);
    pr_info("Read Locked\n");
    /* Read from something */  
    read_unlock_irqrestore(&myrwlock, flags);
    pr_info("Read Unlocked\n");  

}

static void my_write_lock(void){
    unsigned long flags;  
    write_lock_irqsave(&myrwlock, flags);
    pr_info("Write Locked\n");
    /* Write to something */  
    write_unlock_irqrestore(&myrwlock, flags);  
    pr_info("Write Unlocked\n");  
}


static int __init my_rwlock_init(void)  
{  
    pr_info("my_rwlock started\n");  

    my_read_lock();  
    my_write_lock();  
  
    return 0;  
}  



static void __exit my_rwlock_exit(void)  
{  
    pr_info("my_rwlock exit\n");
 
}  
  

module_init(my_rwlock_init);  
module_exit(my_rwlock_exit);  
 
MODULE_DESCRIPTION("Read/Write locks example"); 
MODULE_LICENSE("GPL");
