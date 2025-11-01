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

static int __init my_module_init(void){
    printk(KERN_INFO "Hello, my name is %s\n", my_name);
    printk(KERN_INFO "I am %hd years old\n", my_age);
    printk(KERN_INFO "My hight is %d cm\n", my_hight);
    printk(KERN_INFO "I have %ld money amount\n", money_amount);
    return 0;
}
module_init(my_module_init);
