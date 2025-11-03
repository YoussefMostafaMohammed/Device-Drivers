#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/printk.h>

static DEFINE_MUTEX(mymutex);

static int __init my_mutex_init(void){
    int ret;
    pr_info("example_mutex init");
    ret = mutex_trylock(&mymutex);
    if(ret!=0){
        pr_info("mutex locked");
        mutex_unlock(&mymutex);
        pr_info("mutex is unlocked");
                pr_info("mutex is unlocked");

                        pr_info("mutex is unlocked");

                                pr_info("mutex is unlocked");

    }else{
        pr_info("mutex failed to lock");
    }
    return 0;
}

static void __exit my_mutex_exit(void)  
{  
    pr_info("example_mutex exit\n");
}  

module_init(my_mutex_init);  
module_exit(my_mutex_exit);  

MODULE_DESCRIPTION("Mutex example"); 
MODULE_LICENSE("GPL");