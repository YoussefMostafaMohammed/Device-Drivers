#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

extern char *my_name;
extern short int my_age;
extern int my_hight;
extern long int money_amount;


void my_module_exit(void){
    printk(KERN_INFO "Goodbye from %s\n", my_name);
    return;
}
module_exit(my_module_exit);
