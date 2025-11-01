#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>


MODULE_LICENSE("GPL");
char *my_name = "Ahmed";
short int my_age=20;
int my_hight=188;
long int money_amount=1000000;

module_param(my_name, charp,0644);
MODULE_PARM_DESC(my_name, "This is a character pointer parameter for name");

module_param(my_age, short,0644);
MODULE_PARM_DESC(my_age, "This is a short integer parameter for age");

module_param(my_hight, int,0644);
MODULE_PARM_DESC(my_hight, "This is an integer parameter for height");

module_param(money_amount, long,0644);
MODULE_PARM_DESC(money_amount, "This is a long integer parameter for money amount");

