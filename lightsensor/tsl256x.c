/*
 * tsl256x.c 
 *
 * Revision history:
 * 29 Jun 2009: initial version
 *
 */
#include <linux/workqueue.h>
#include <linux/spinlock.h>

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>

//#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include <linux/sysctl.h>
#include <asm/uaccess.h>

#include "tsl256x.h"

// Device Information
#define LIGHTSENSOR_MAJOR 150

struct file_operations tsl256x_fops;

// Addresses to scan 
// This depends on the hardward design
// 0x29: GND activated mode
// 0x39:
// 0x49:
static unsigned short normal_i2c[] = { 0x29, I2C_CLIENT_END };

// Macro to add other variables for I2C drivers
I2C_CLIENT_INSMOD_1(tsl256x);

/* Each client has this additional data */
struct tsl256x_data {
	struct i2c_client client;
	struct mutex update_lock;
};
//static spinlock_t sensor_lock=SPIN_LOCK_UNLOCKED;

static struct i2c_driver tsl256x_driver;
static struct device *tsl256x_device;

/*
 * I2C Functions
 */
static int tsl256x_attach_adapter(struct i2c_adapter *adapter);
static int tsl256x_probe(struct i2c_adapter *adapter, int address, int kind);
static int tsl256x_detach_client(struct i2c_client *client);
static int tsl256x_init_client(struct i2c_client *client);

static int read_lightsensor(int *LI);

//-----------------------------------------------------------------------------------
static int tsl256x_open(struct inode *inode, struct file *file)
{
	return 0;
}


static int tsl256x_release(struct inode *inode, struct file *file)
{
	return 0;
}


static ssize_t tsl256x_read(struct file *file, char *buf, size_t count,
                loff_t *ptr)
{
	return 0;
}


static ssize_t tsl256x_write(struct file *file, const char *buf,
                size_t count, loff_t * ppos)
{
	return 0;
}

static int tsl256x_ioctl(struct inode *inode, struct file *file,
                unsigned int cmd, unsigned long arg)
{
	int ret=0;

	switch (cmd)
	{
		case CMD_LI_READ:
			ret = read_lightsensor((int*)arg);
		break;
		default:
			ret = -1;
		break;
	}
	return ret;
}

struct file_operations tsl256x_fops = {
        owner:   THIS_MODULE,
        read:    tsl256x_read,
        write:   tsl256x_write,
        ioctl:   tsl256x_ioctl,
        open:    tsl256x_open,
        release: tsl256x_release,
};
//-----------------------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////////
// unsigned int calculateLux(unsigned int ch0, unsigned int ch0, int iType)
//
// Description: Calculate the approximate illuminance (lux) given the raw
// channel values of the TSL2560. The equation if implemented
// as a piece-wise linear approximation.
//
// Arguments: unsigned int iGain - gain, where 0:1X, 1:16X
// unsigned int tInt - integration time, where 0:13.7mS, 1:100mS, 2:402mS,
// 3:Manual
// unsigned int ch0 - raw channel value from channel 0 of TSL2560
// unsigned int ch1 - raw channel value from channel 1 of TSL2560
// unsigned int iType - package type (T, CL, FN, or CS)
//
// Return: unsigned int - the approximate illuminance (lux)
//
// Reference to: TSL256X chip manual
//////////////////////////////////////////////////////////////////////////////
static unsigned int calculateLux(unsigned int iGain, unsigned int tInt, 
			  unsigned int ch0, unsigned int ch1, int iType)
{
	//--------------------------------------------------------------
	// first, scale the channel values depending on 
	// the gain and integration time
	// 16X, 402mS is nominal.
	// scale if integration time is NOT 402 msec
	unsigned long chScale;
	unsigned long channel1;
	unsigned long channel0;

	// ratio and the channel values (Channel1/Channel0)
	unsigned long ratio1 = 0;
	unsigned long ratio = 0;
	unsigned int b=0, m=0;

	unsigned long temp;
	unsigned long lux = 0;

	switch (tInt)
	{
		case 0: // 13.7 msec
			chScale = CHSCALE_TINT0;
			break;
		case 1: // 101 msec
			chScale = CHSCALE_TINT1;
			break;
		default: // assume no scaling
			chScale = (1 << CH_SCALE);
			break;
	}
	// scale if gain is NOT 16X
	if (!iGain) chScale = chScale << 4; // scale 1X to 16X
	// scale the channel values
	channel0 = (ch0 * chScale) >> CH_SCALE;
	channel1 = (ch1 * chScale) >> CH_SCALE;

	//-----------------------------------------------------------------
	// find the ratio of the channel values (Channel1/Channel0)
	// protect against divide by zero
	if (channel0 != 0) ratio1 = (channel1 << (RATIO_SCALE+1)) / channel0;
	// round the ratio value
	ratio = (ratio1 + 1) >> 1;

	// is ratio <= eachBreak ?
	switch (iType)
	{
		case 0: // T, CL,or FN package
			if ((ratio >= 0) && (ratio <= K1T))
			{b=B1T; m=M1T;}
			else if (ratio <= K2T)
			{b=B2T; m=M2T;}
			else if (ratio <= K3T)
			{b=B3T; m=M3T;}
			else if (ratio <= K4T)
			{b=B4T; m=M4T;}
			else if (ratio <= K5T)
			{b=B5T; m=M5T;}
			else if (ratio <= K6T)
			{b=B6T; m=M6T;}
			else if (ratio <= K7T)
			{b=B7T; m=M7T;}
			else if (ratio > K8T)
			{b=B8T; m=M8T;}
			break;
		case 1:// CS package
			if ((ratio >= 0) && (ratio <= K1C))
			{b=B1C; m=M1C;}
			else if (ratio <= K2C)
			{b=B2C; m=M2C;}
			else if (ratio <= K3C)
			{b=B3C; m=M3C;}
			else if (ratio <= K4C)
			{b=B4C; m=M4C;}
			else if (ratio <= K5C)
			{b=B5C; m=M5C;}
			else if (ratio <= K6C)
			{b=B6C; m=M6C;}
			else if (ratio <= K7C)
			{b=B7C; m=M7C;}
			else if (ratio > K8C)
			{b=B8C; m=M8C;}
			break;
	}

	temp = ((channel0 * b) - (channel1 * m));
	// do not allow negative lux value
	if (temp < 0) temp = 0;
	// round lsb (2^(LUX_SCALE-1))
	temp += (1 << (LUX_SCALE-1));
	// strip off fractional portion
	lux = temp >> LUX_SCALE;
	return(lux);
}


static int read_lightsensor(int *LI)
{
	int data0low=0, data0high=0, data1low=0, data1high=0;
	int data0=0, data1=0;
	int value=0, ret=0;
	struct i2c_client *client = NULL;
	struct tsl256x_data *clientdata = NULL;

	if (tsl256x_device == NULL){
		printk("tsl256x_device = NULL!\n");
		value = calculateLux(0, 2, data0, data1, 0);
		copy_to_user(LI, &value, sizeof(int));
		return -1;
	}
	client = to_i2c_client(tsl256x_device);
	if (client == NULL){
		printk("i2c_client = NULL !\n");
                value = calculateLux(0, 2, data0, data1, 0);
                copy_to_user(LI, &value, sizeof(int));
                return -1;
        }
	clientdata = i2c_get_clientdata(client);
	if (clientdata == NULL){
		printk("clientdata = NULL !\n");
                value = calculateLux(0, 2, data0, data1, 0);
                copy_to_user(LI, &value, sizeof(int));
                return -1;
        }



	//unsigned long flags;
	
	mutex_lock(&clientdata->update_lock);
	//spin_lock(&sensor_lock, flags);
	data0low = i2c_smbus_read_byte_data(client, TSL256X_REG_DATA0LOW);
	data0high = i2c_smbus_read_byte_data(client, TSL256X_REG_DATA0HIGH);
	data1low = i2c_smbus_read_byte_data(client, TSL256X_REG_DATA1LOW);
	data1high = i2c_smbus_read_byte_data(client, TSL256X_REG_DATA1HIGH);
	mutex_unlock(&clientdata->update_lock);
	//spin_unlock_irqrestore(&sensor_lock, flags);
	
	if ( (data0low<0)||(data0high<0)||(data1low<0)||(data1high<0) ) {
		printk("i2c error !!!\n");
		//printk("data0low=%x, data0hight=%x, data1low=%x, data1high=%x\n", data0low, data0high, data1low, data1high);
		return -1;
	}

	data0 = data0high << 8 | data0low;
	data1 = data1high << 8 | data1low;
	
	//printk("data0low=%x, data0hight=%x, data1low=%x, data1high=%x: ", data0low, data0high, data1low, data1high);
	//printk("data0=%x, data1=%x\n", data0, data1);
	value = calculateLux(0, 2, data0, data1, 0);

	//printk("LI=%u\n", value);
	ret = copy_to_user(LI, &value, sizeof(int));
	
	return 0;
}

/*
 * Initialization function
 */
static int tsl256x_init_client(struct i2c_client *client)
{
        int err;
	int value;

        /*
         * Probe the chip. To do so we try to power up the device and then to
         * read back the 0x03 code
         */
        err = i2c_smbus_write_byte_data(client, TSL256X_REG_CONTROL, TSL256X_POWER_DOWN);
        mdelay(100);
        err = i2c_smbus_write_byte_data(client, TSL256X_REG_CONTROL, TSL256X_POWER_UP);
        if (err < 0) {
		printk(KERN_DEBUG "i2c_smbus_write_byte_data error %d\n", err);
                return err;
	}
        mdelay(1);
	value = i2c_smbus_read_byte_data(client, TSL256X_REG_CONTROL);
        if (value != TSL256X_POWER_UP) {
		printk(KERN_DEBUG "i2c_smbus_read_byte_data error 0x%x\n", value);
                return -ENODEV;
	}
	
	// Get ID from the chip
	// Just do it as usual, can skip this following part
        mdelay(1);
	value = i2c_smbus_read_byte_data(client, TSL256X_REG_ID);
	printk(KERN_INFO "TSL256X ID = 0x%xh\n", value);

        return 0;
}

//------------------------------------------------------------------------------------------
/* This function is called by i2c_probe */
static int tsl256x_probe(struct i2c_adapter *adapter, int address, int kind)
{
	struct i2c_client *new_client = NULL;
	struct tsl256x_data *clientdata = NULL;
	int err = 0;

	printk("tsl256x_probe()!\n");

	tsl256x_device = NULL;

	// Check I2C Adapter readiness
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		printk(KERN_DEBUG "i2c_check_functionalty...error\n");
		goto exit;
	}

	// get kernal memory
	if (!(clientdata = kzalloc(sizeof(struct tsl256x_data), GFP_KERNEL))) {
		printk(KERN_DEBUG "kzalloc.. error\n");
		err = -ENOMEM;
		goto exit;
	}

	mutex_init(&clientdata->update_lock);
	//spin_lock_init(&sensor_lock);
	new_client = &clientdata->client;
	i2c_set_clientdata(new_client, clientdata);
	new_client->addr = address;
	printk("I2c address=%x\n", address);
	new_client->adapter = adapter;
	new_client->driver = &tsl256x_driver;
	new_client->flags = 0;
	strlcpy(new_client->name, "tsl256x", I2C_NAME_SIZE);

	// Tell the I2C layer a new client has arrived 
	if ((err = i2c_attach_client(new_client))) {
		printk(KERN_DEBUG "i2c_attach_client Error..%s\n", new_client->name);
		goto exit_free;
	}
	
	tsl256x_device = &new_client->dev;

	err = tsl256x_init_client(new_client);
	if (err) {
		printk(KERN_DEBUG "tsl256x_init_client error\n");
		goto exit_free;
	}
	
	return 0;

exit_free:
	if (clientdata != NULL)
		kfree(clientdata);
exit:
	printk(KERN_DEBUG "tsl256x driver error\n");
	return err;
}

static int tsl256x_attach_adapter(struct i2c_adapter *adapter)
{
	return i2c_probe(adapter, &addr_data, tsl256x_probe);
}

static int tsl256x_detach_client(struct i2c_client *client)
{
	struct tsl256x_data *data = i2c_get_clientdata(client);
	int err=0;
	int value;

	// Powerdown the lightsensor chip
	i2c_smbus_write_byte_data(client, TSL256X_REG_CONTROL, TSL256X_POWER_DOWN);
	value = i2c_smbus_read_byte_data(client, TSL256X_REG_CONTROL);
	if (value == TSL256X_POWER_DOWN) {
		printk(KERN_DEBUG "tsl256x power down successfully\n");
	}

	// Detach I2C Client
	err = i2c_detach_client(client);
	if (data != NULL) {
		kfree(data);
	}
	return err;
}

/* This is the driver that will be inserted */
static struct i2c_driver tsl256x_driver = {
	.driver = {
		.name		= "tsl256x",
		.owner		= THIS_MODULE,
	},
	.id		= I2C_DRIVERID_TSL256X,
	.attach_adapter	= tsl256x_attach_adapter,
	.detach_client	= tsl256x_detach_client,
};

static int __init tsl256x_init(void)
{
	int ret;

	printk("tsl256x_init start..\n");
	ret = register_chrdev(LIGHTSENSOR_MAJOR, "lightsensor", &tsl256x_fops);
	if (ret < 0){
		unregister_chrdev(LIGHTSENSOR_MAJOR, "lightsensor");
		printk("init error!\n");
		return ret;
	}
	ret = i2c_add_driver(&tsl256x_driver);

	printk(KERN_INFO "tsl256x_init status = %xh\n", ret);
	
	return ret;
}

static void __exit tsl256x_exit(void)
{
	printk(KERN_INFO "tsl256x_exit\n");
	i2c_del_driver(&tsl256x_driver);

	unregister_chrdev(LIGHTSENSOR_MAJOR, "lightsensor");
}

MODULE_AUTHOR (" ");
MODULE_DESCRIPTION("TSL256x Light Sensor Driver");
MODULE_LICENSE("GPL v2");

module_init(tsl256x_init);
module_exit(tsl256x_exit);
