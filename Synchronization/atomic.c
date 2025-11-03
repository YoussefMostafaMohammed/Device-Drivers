#include <linux/atomic.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/printk.h>


#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
 
#define BYTE_TO_BINARY(byte)                                                   \
    ((byte & 0x80) ? '1' : '0'), ((byte & 0x40) ? '1' : '0'),                  \
        ((byte & 0x20) ? '1' : '0'), ((byte & 0x10) ? '1' : '0'),              \
        ((byte & 0x08) ? '1' : '0'), ((byte & 0x04) ? '1' : '0'),              \
        ((byte & 0x02) ? '1' : '0'), ((byte & 0x01) ? '1' : '0')


atomic_t age; 
atomic_t year=ATOMIC_INIT(25);

static void atomic_add_subtract(void){
    
    atomic_set(&age,10);
    
    atomic_dec(&age); // subtract one

    atomic_inc(&year); // increase one
    
    atomic_add(atomic_read(&age) ,&year);

    pr_info("chris: %d, debbie: %d\n", atomic_read(&year),  
            atomic_read(&age));
    
    return;
}


static void atomic_bitwise(void){
    unsigned long word = 0;  

    pr_info("Bits 0: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word));  
    set_bit(3, &word);
 
    set_bit(5, &word);  
    pr_info("Bits 1: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word));
 
    clear_bit(5, &word);  
    pr_info("Bits 2: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word));
 
    change_bit(3, &word);  
  
    pr_info("Bits 3: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word));
 
    if (test_and_set_bit(3, &word))  
        pr_info("wrong\n");
 
    pr_info("Bits 4: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(word));  
  
    word = 255;
 
    pr_info("Bits 5: " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(word));  

    return;
}


static int __init my_atomic_init(void){
    pr_info("my_atomic started\n");
 
    atomic_add_subtract();  
    atomic_bitwise();  
  
    return 0;  
}


static void __exit my_atomic_exit(void){
    pr_info("my_atomic exit\n");
    return;
}

  
module_init(my_atomic_init);  
module_exit(my_atomic_exit);  

MODULE_DESCRIPTION("Atomic operations example"); 
MODULE_LICENSE("GPL");
