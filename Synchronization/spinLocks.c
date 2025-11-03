#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/printk.h>

/*
As the name suggests, spinlocks lock up the CPU that the code is 
running on,taking 100% of its resources.Because of this you
should only use the spinlock mechanism around code which
is likely to take no more than a few milliseconds
to run and so will not noticeably slow anything
down from the user’s point of view.

في الكيرنل، الـ atomic context هو أي حالة ما ينفعش فيها الـ thread ينام أو يتوقف (sleep)
، لأن الكيرنل بيعتبر العملية “حساسة” ولازم تكمل فورًا.
*/

static DEFINE_SPINLOCK(myspinlock_static);

static spinlock_t myspinlock_dynamic;

static void my_spinlock_static(void){
    unsigned long flags;
    spin_lock_irqsave(&myspinlock_static,flags);
    pr_info("locked static spinlock\n");
    // safe code 
    spin_unlock_irqrestore(&myspinlock_static,flags);
    pr_info("unlock static spinlock\n");
}


 
static void my_spinlock_dynamic(void)  
{  
    unsigned long flags;  
  
    spin_lock_init(&myspinlock_dynamic);
 
    spin_lock_irqsave(&myspinlock_dynamic, flags);  
    pr_info("Locked dynamic spinlock\n");
    // safe code
    spin_unlock_irqrestore(&myspinlock_dynamic, flags);  
    pr_info("Unlocked dynamic spinlock\n");  
}
 
  
static int __init my_spinlock_init(void)  
{  
    pr_info("example spinlock started\n");
 
  
    my_spinlock_static();  
    my_spinlock_dynamic();  
  
    return 0;  
}  


static void __exit my_spinlock_exit(void)  
{  
    pr_info("example spinlock exit\n");
                                                                  

                                                                  
 
}  
module_init(my_spinlock_init);  
module_exit(my_spinlock_exit); 

MODULE_DESCRIPTION("Spinlock example"); 
MODULE_LICENSE("GPL");


