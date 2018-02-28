#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>

#include <linux/module.h>
//#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sysctl.h>

#include <asm/uaccess.h>
#include <asm/gpio.h>
#include <linux/clk.h>

#include <mach/at91sam9263.h>
#include <linux/i2c.h>

#include "touchkey.h"

#define BCP_MAINBOARD	0
#define DDU_MAINBOARD	1

#define MAINBOARD	DDU_MAINBOARD

//----------------------------------------------------------------------------
/* # mknod /dev/touchkey c 151 0 */
// Device information
#define TOUCHKEY_MAJOR 151

struct file_operations touchkey_fops;
//----------------------------------------------------------------------------
// Slave address
#define TOUCHKEY_ADDRESS 0x2A 

#define TOUCHKEY_INT_PIN	AT91_PIN_PE8
#define TOUCHKEY_DEBOUNCE_TIME	30
#define TOUCHKEY_HOLD_TIME	10
#define TOUCHKEY_TIMEOUT	400

struct touchkey_data_t {
	struct i2c_client client;
	spinlock_t	lock;
};

/*
 * Generic i2c probe
 */
static unsigned short normal_i2c[] = {
	TOUCHKEY_ADDRESS,
	I2C_CLIENT_END,
};

// Macro to add other variables for I2C drivers
I2C_CLIENT_INSMOD_1(touchkey);


static struct i2c_driver touchkey_driver;
static struct device *touchkey_device;
static spinlock_t touchkey_lock=SPIN_LOCK_UNLOCKED;

static int touchkey_attach_adapter(struct i2c_adapter *adapter);
static int touchkey_detach_client(struct i2c_client *client);


//----------------------------------------------------------------------------------
#define DDU_TOTAL_KEY	22
#define BCP_TOTAL_KEY	5

typedef	enum {
	DUMMY_DATA,
	KEY_MESSAGE,   // read key value
	KEY_SET_REF,     // Set Ref Value 
	KEY_SET_FTH     // Set the Finger Threshold of each button sensor, Useful when different sensors are   
					// placed on different locations and some sensors are more sensitive than others.
}PrimitiveType;
 
typedef	enum {
	PRIMITIVE = 0x00, 
	REF_VALUE, 
	BUTTON, 
	FINGER_TH = 0x06, 
	RAW_COUNT_LSB, 
	RAW_COUNT_MSB, 
	DIFF_COUNT_LSB, 
	DIFF_COUNT_MSB, 
	BASE_COUNT_LSB, 
	BASE_COUNT_MSB, 
	PSOC_ID = 0x0F
}I2cReadBuffer;

static key_info touch_key;
static unsigned long starttime=0;
static unsigned char total_key=0;
static int keypad_mode;

static unsigned char keyflag=0;

//-----------------------------------------------------------------------------------
static int touchkey_open(struct inode *inode, struct file *file)
{
	return 0;
}


static int touchkey_release(struct inode *inode, struct file *file)
{
	return 0;
}


static ssize_t touchkey_read(struct file *file, char *buf, size_t count,
                loff_t *ptr)
{
	return 0;
}


static ssize_t touchkey_write(struct file *file, const char *buf,
                size_t count, loff_t * ppos)
{
	return 0;
}

static int touchkey_get_touchkey(key_info *pkey)
{
	int ret;
	unsigned long time;

	if (touch_key.keytype==KEY_PRESS_EVENT)//new key pressed
	{
		time = jiffies;
        	time = time - starttime;//bound
	        if(time < TOUCHKEY_TIMEOUT){
			touch_key.duration = time;
		}
                else{
                	touch_key.keytype=KEY_RELEASE_EVENT;
                	printk("Duration>%d! Touch key timeout...\n", time);
                }
	}
    else//key released
	{}
	if (touch_key.duration > TOUCHKEY_HOLD_TIME){
		ret = copy_to_user(&pkey->keytype, &touch_key.keytype, sizeof(unsigned char));
		ret = copy_to_user(&pkey->keycode, &touch_key.keycode, sizeof(unsigned char));
		ret = copy_to_user(&pkey->duration, &touch_key.duration, sizeof(unsigned long));
	}

	if (touch_key.keytype==KEY_RELEASE_EVENT)//key release
	{
		touch_key.keytype = NO_KEY_EVENT;
        touch_key.keycode = 0;
        touch_key.duration = 0;
	}

    return ret;
}

static int touchkey_ioctl(struct inode *inode, struct file *file,
                unsigned int cmd, unsigned long arg)
{
	int ret=0;

	switch (cmd)
	{
		case CMD_TOUCHKEY_INIT:
			printk("touchkey init!!!!!!\n");
			ret = keypad_mode;

			// clear storage
			touch_key.keytype = 0;
			touch_key.keycode = 0;
			touch_key.duration = 0;
			starttime = 0;
			keyflag = 0;
			break;
		case CMD_TOUCH_STARTREAD:
			keyflag = 0;
		break;
		case CMD_TOUCH_STOPREAD:
			keyflag = 1;
		break;
		case CMD_TOUCH_GETKEY:
			touchkey_get_touchkey((key_info*)arg);
			ret=0;
			break;
		default:
			ret = -1;
		break;
	}
	return ret;
}

struct file_operations touchkey_fops = {
        owner:   THIS_MODULE,
        read:    touchkey_read,
        write:   touchkey_write,
        ioctl:   touchkey_ioctl,
        open:    touchkey_open,
        release: touchkey_release,
};
//-----------------------------------------------------------------------------------

static int touchkey_event_handle(unsigned char key)
{
        //unsigned long timeout = jiffies + HZ/2; // timeout in 0.5s 
        unsigned long time;

        if (key!=0)//new key press
        {
			if (starttime==0)
			{
                touch_key.keytype = KEY_PRESS_EVENT;
                touch_key.keycode = key;
                touch_key.duration = 0;
                starttime = jiffies;
				printk("Button %d Pressed ...\n\r",key);
			}
        }
        else//key release
        {
				touch_key.keytype = KEY_RELEASE_EVENT;
                time = jiffies;
                time -= starttime;//bound
                touch_key.duration = time;
				starttime = 0;
				printk("Button Released ... time=%ld\n", time);
        }

        return 0;
}

static irqreturn_t touchkey_isr(int irq, void *dev_id)
{
	unsigned char keyValue = 0;
	int i=0, ret=0, irqret=IRQ_NONE;

	struct i2c_client *client = NULL;
	unsigned char I2CBuf[16];
	unsigned int button;
	unsigned long flags;

	spin_lock_irqsave(&touchkey_lock, flags);

	if (keyflag==0) {
//printk("IRQ! ");
		
		if (touchkey_device!=NULL)
	                client = to_i2c_client(touchkey_device);

		memset(I2CBuf, 0, sizeof(I2CBuf));
		if (client!=NULL)
			ret = i2c_master_recv(client, I2CBuf, sizeof(I2CBuf));
//printk("ret=%d!\n", ret);
		if (ret>0){	

			//for (i=0;i<16;i++)
			//	printk("[%x] ", I2CBuf[i]);
			//printk("\n");

			if (I2CBuf[PRIMITIVE]==KEY_MESSAGE){
				button = I2CBuf[BUTTON] | (I2CBuf[BUTTON+1]<<8) | (I2CBuf[BUTTON+2]<<16) | (I2CBuf[BUTTON+3]<<24) ;
				//printk("button=%x\n",button);
				for (i=0;i<total_key;i++) {
					if ((button>>i)==1) {
						//printk("-I- Button %d Press ...(%x)\n",i, button);
						keyValue = i+1;
					}
				}
				touchkey_event_handle(keyValue);
				irqret = IRQ_HANDLED;
			}
		}
	}
	spin_unlock_irqrestore(&touchkey_lock, flags);


	return irqret;
}

//-----------------------------------------------------------------------------------

static int touchkey_initstart(void)
{
	int error=0;
	struct i2c_client *client = NULL;
	unsigned char I2CBuf[16];
	unsigned long flags;
	
	//Begin of Mantis #6992: Startup failure if touch sense keypad initialization failure
	//Reset touchkeypad with active HIGH XRES for CY8C21345
    at91_set_gpio_output(AT91_PIN_PE7, 1);
    mdelay(100);
    at91_set_gpio_output(AT91_PIN_PE7, 0);
    mdelay(400);
	//End of Mantis #6992
	
	if (touchkey_device!=NULL)
		client = to_i2c_client(touchkey_device);

	//Get info
	memset(I2CBuf, 0, sizeof(I2CBuf));
	if (client !=NULL)
		error = i2c_master_recv(client, I2CBuf, sizeof(I2CBuf));

	if (error>0){
		if (I2CBuf[PSOC_ID]==DDU_ID) {
			keypad_mode = TOUCHKEY_TYPE_DDU;
			total_key = DDU_TOTAL_KEY;
			printk("DDU\n");
		}
		else if (I2CBuf[PSOC_ID]==BCP_ID) {
			keypad_mode = TOUCHKEY_TYPE_BCP;
			total_key = BCP_TOTAL_KEY;
			printk("BCP\n");
		}
		else {
			printk("touch-sensitive keypad: get info fail, id=%d\n",I2CBuf[PSOC_ID]);
			goto fail;
		}
	} else {
		printk("touch-sensitive keypad: get info fail, id=%d\n",I2CBuf[PSOC_ID]);
		goto fail;
	}
	
	//Set irq pin
	spin_lock_irqsave(&touchkey_lock, flags);
	keyflag = 1	;
	error = request_irq(AT91_PIN_PE8, touchkey_isr,0,"touch-sensitive keypad",NULL);
	if (error<0) {
		printk("touch-sensitive keypad: Unable to claim irq,error %d\n",error);
		goto fail;
	}
	spin_unlock_irqrestore(&touchkey_lock, flags);

	// init variables
	touch_key.keytype = 0;
	touch_key.keycode = 0;
	touch_key.duration = 0;
	starttime = 0;
	return 0;
	
	fail:
	return error;
}

static int touchkey_end(void)
{
	free_irq(AT91_PIN_PE8, NULL);
	gpio_free(AT91_PIN_PE8);
	return 0;
}

//-------------------------------------------------------------------------------------
static int touchkey_probe(struct i2c_adapter *adapter, int address, int kind)
{
	struct i2c_client *new_client = NULL;
	struct touchkey_data_t *clientdata = NULL;
	int err = 0;

printk("touchkey_probe()!\n");
	touchkey_device = NULL;

	// Check I2C Adapter readiness
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		printk(KERN_DEBUG "i2c_check_functionalty...error\n");
		goto exit;
	}

	// get kernal memory
	if (!(clientdata = kzalloc(sizeof(struct touchkey_data_t), GFP_KERNEL))) {
		printk(KERN_DEBUG "kzalloc.. error\n");
		err = -ENOMEM;
		goto exit;
	}

	spin_lock_init(&touchkey_lock);

	new_client = &clientdata->client;
	i2c_set_clientdata(new_client, clientdata);
	new_client->addr = address;
	printk("I2c address=%x\n", address);
	new_client->adapter = adapter;
	new_client->driver = &touchkey_driver;
	new_client->flags = 0;
	strlcpy(new_client->name, "touchkey", I2C_NAME_SIZE);

	// Tell the I2C layer a new client has arrived 
	if ((err = i2c_attach_client(new_client))) {
		printk(KERN_DEBUG "i2c_attach_client Error..%s\n", new_client->name);
		goto exit_free;
	}

	touchkey_device = &new_client->dev;

	err = touchkey_initstart();
	if (err!=0) {
		printk(KERN_DEBUG "touchkey_initstart error\n");
		goto exit_free;
	}

	return 0;

exit_free:
	if (clientdata != NULL)
		kfree(clientdata);

exit:
	printk(KERN_DEBUG "touchkey driver error\n");
	return err;
}

static int touchkey_attach_adapter(struct i2c_adapter *adapter)
{
	return i2c_probe(adapter, &addr_data, touchkey_probe);
}

static int touchkey_detach_client(struct i2c_client *client)
{
	struct touchkey_data_t *data = i2c_get_clientdata(client);
	int err;

	touchkey_end();

	err = i2c_detach_client(client);
	printk("i2c_detach_client status=%d\n", err);
    if (data != NULL)
        kfree(data);

	return err;
}

/* This is the driver that will be inserted */
static struct i2c_driver touchkey_driver = {
	.driver = {
		.name		= "touchkey",
		.owner		= THIS_MODULE,
	},
	.id		= I2C_DRIVERID_CY8C22545, //I2C_DRIVERID_TOUCHKEY,
	.attach_adapter	= touchkey_attach_adapter,
	.detach_client	= touchkey_detach_client,
};

static int __init touchkey_init(void)
{
	int ret;

	printk("touchkey_init start..\n");
	ret = register_chrdev(TOUCHKEY_MAJOR, "touchkey", &touchkey_fops);
	if (ret < 0){
		unregister_chrdev(TOUCHKEY_MAJOR, "touchkey");
		printk("init error!\n");
		return ret;
	}

	ret = i2c_add_driver(&touchkey_driver);
	printk("touchkey init status = %xh\n", ret);

	printk("touchkey_init OK!\n");

	return 0;
}

static void __exit touchkey_exit(void)
{
	printk("touchkey_exit\n");

	i2c_del_driver(&touchkey_driver);

	unregister_chrdev(TOUCHKEY_MAJOR, "touchkey");
}

module_init(touchkey_init);
module_exit(touchkey_exit);

MODULE_AUTHOR (" ");
MODULE_DESCRIPTION("TOUCHKEY Driver");
MODULE_LICENSE("GPL v2");
