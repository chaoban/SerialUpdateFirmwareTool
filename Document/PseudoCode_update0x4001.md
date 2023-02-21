# Update Firmware Pseudo Code

> **Update Flow**
>
> 1.	**Switch FW Mode**
> 2.	**Get FW Information**
> 3.	**Check FW Info**
> 4.	**Check BootFlag**
> 5.	**Check Bootloader ID and Bootloader CRC**
> 6.	**Update FW**



### I2C Read Write Commands

```c
static int sis_command_for_write(struct i2c_client *client, int wlength,
							unsigned char *wdata)
{
	int ret = SIS_ERR;
	struct i2c_msg msg[1];

	msg[0].addr = client->addr;
	msg[0].flags = 0;/*Write*/
	msg[0].len = wlength;
	msg[0].buf = (unsigned char *)wdata;
	ret = i2c_transfer(client->adapter, msg, 1);
	return ret;

}

static int sis_command_for_read(struct i2c_client *client, int rlength,
							unsigned char *rdata)
{
	int ret = SIS_ERR;
	struct i2c_msg msg[1];

	msg[0].addr = client->addr;
	msg[0].flags = I2C_M_RD;/*Read*/
	msg[0].len = rlength;
	msg[0].buf = rdata;
	ret = i2c_transfer(client->adapter, msg, 1);
	return ret;

}
```



### Read Date By I2C Bus

```C
static int sis_ReadPacket(struct i2c_client *client, uint8_t cmd, uint8_t *buf)
{
	uint8_t tmpbuf[MAX_BYTE] = {0};	/*MAX_BYTE = 64;*/
#ifdef _CHECK_CRC
	uint16_t buf_crc = 0;
	uint16_t package_crc = 0;
	int l_package_crc = 0;
	int crc_end = 0;
#endif /* _CHECK_CRC */
	int ret = SIS_ERR;
	int touchnum = 0;
	int p_count = 0;
	int touc_formate_id = 0;
	int locate = 0;
	bool read_first = true;
	/*

 * New i2c format
   * buf[0] = Low 8 bits of byte count value
   * buf[1] = High 8 bits of byte counte value
   * buf[2] = Report ID
   * buf[touch num * 6 + 2 ] = Touch informations;
   * 1 touch point has 6 bytes, it could be none if no touch
   * buf[touch num * 6 + 3] = Touch numbers
     *
   * One touch point information include 6 bytes, the order is
     *
   * 1. status = touch down or touch up
   * 2. id = finger id
   * 3. x axis low 8 bits
   * 4. x axis high 8 bits
   * 5. y axis low 8 bits
   * 6. y axis high 8 bits
   * */
     do {
     if (locate >= PACKET_BUFFER_SIZE) {
     	pr_err("sis_ReadPacket: Buf Overflow\n");
     	return SIS_ERR;
     }
     ret = sis_command_for_read(client, MAX_BYTE, tmpbuf);



		if (ret < 0) {
			pr_err("sis_ReadPacket: i2c transfer error\n");
			return SIS_ERR_TRANSMIT_I2C;
		}
		/*error package length of receiving data*/
		else if (tmpbuf[P_BYTECOUNT] > MAX_BYTE) {
	
			return SIS_ERR;
		}
		if (read_first) {
			/* access NO TOUCH event unless BUTTON NO TOUCH event*/
			if (tmpbuf[P_BYTECOUNT] == 0/*NO_TOUCH_BYTECOUNT*/)
				return touchnum;/*touchnum is 0*/
			if (tmpbuf[P_BYTECOUNT] == 3/*EMPRY PACKET*/) {
	
				return SIS_ERR_EMPTY_PACKET;
			}
		}
		/*skip parsing data when two devices are registered
		 * at the same slave address*/
		/*parsing data when P_REPORT_ID && 0xf is TOUCH_FORMAT
		 * or P_REPORT_ID is ALL_IN_ONE_PACKAGE*/
		touc_formate_id = tmpbuf[P_REPORT_ID] & 0xf;
	
		if ((touc_formate_id != HIDI2C_FORMAT)

#ifndef CONFIG_TOUCHSCREEN_SIS_I2C_95XX
		&& (touc_formate_id != TOUCH_FORMAT)
#endif
		&& (touc_formate_id != BUTTON_FORMAT)
		&& (tmpbuf[P_REPORT_ID] != ALL_IN_ONE_PACKAGE)) {
			pr_err("sis_ReadPacket: Error Report_ID 0x%x\n",
				tmpbuf[P_REPORT_ID]);
			return SIS_ERR;
		}
		p_count = (int) tmpbuf[P_BYTECOUNT] - 1;	/*start from 0*/
		if (tmpbuf[P_REPORT_ID] != ALL_IN_ONE_PACKAGE) {
			if (touc_formate_id == HIDI2C_FORMAT) {
				p_count -= BYTE_CRC_HIDI2C;
#ifndef CONFIG_TOUCHSCREEN_SIS_I2C_95XX
			} else if (touc_formate_id == TOUCH_FORMAT) {
				p_count -= BYTE_CRC_I2C;/*delete 2 byte crc*/
#endif
			} else if (touc_formate_id == BUTTON_FORMAT) {
				p_count -= BYTE_CRC_BTN;
			} else {	/*should not be happen*/
				pr_err("sis_ReadPacket: delete crc error\n");
				return SIS_ERR;
			}
			if (IS_SCANTIME(tmpbuf[P_REPORT_ID])) {
				p_count -= BYTE_SCANTIME;
#ifdef CONFIG_TOUCHSCREEN_SIS_I2C_95XX
			} else {
				pr_err("sis_ReadPacket: Error Report_ID 0x%x, no scan time\n",
					tmpbuf[P_REPORT_ID]);
				return SIS_ERR;	
#endif
			}
		}
		/*else {}*/ /*For ALL_IN_ONE_PACKAGE*/
		if (read_first)
			touchnum = tmpbuf[p_count];
		else {
			if (tmpbuf[p_count] != 0) {
				pr_err("sis_ReadPacket: get error package\n");
				return SIS_ERR;
			}
		}

#ifdef _CHECK_CRC
		crc_end = p_count + (IS_SCANTIME(tmpbuf[P_REPORT_ID]) * 2);
		buf_crc = cal_crc(tmpbuf, 2, crc_end);
		/*sub bytecount (2 byte)*/
		l_package_crc = p_count + 1
		+ (IS_SCANTIME(tmpbuf[P_REPORT_ID]) * 2);
		package_crc = ((tmpbuf[l_package_crc] & 0xff)
		| ((tmpbuf[l_package_crc + 1] & 0xff) << 8));

		if (buf_crc != package_crc)	{
			pr_err("sis_ReadPacket: CRC Error\n");
			return SIS_ERR;
		}

#endif
		memcpy(&buf[locate], &tmpbuf[0], 64);
		/*Buf_Data [0~63] [64~128]*/
		locate += 64;
		read_first = false;
	} while (tmpbuf[P_REPORT_ID] != ALL_IN_ONE_PACKAGE &&
			tmpbuf[p_count] > 5);
	return touchnum;
}
```



### CRC

```C
uint16_t cal_crc(char *cmd, int start, int end)
{
	int i = 0;
	uint16_t crc = 0;
	for (i = start; i <= end ; i++)
		crc = (crc<<8) ^ crc16tab[((crc>>8) ^ cmd[i])&0x00FF];
	return crc;
}

uint16_t cal_crc_with_cmd (char* data, int start, int end, uint8_t cmd)
{
	int i = 0;
	uint16_t crc = 0;

	crc = (crc<<8) ^ crc16tab[((crc>>8) ^ cmd)&0x00FF];
	for (i = start; i <= end ; i++)
		crc = (crc<<8) ^ crc16tab[((crc>>8) ^ data[i])&0x00FF];
	return crc;

}

void write_crc(unsigned char *buf, int start, int end)
{
	uint16_t crc = 0;
	crc = cal_crc(buf, start , end);
	buf[end+1] = (crc >> 8) & 0xff;
	buf[end+2] = crc & 0xff;
}

static uint8_t sis_calculate_output_crc( u8* buf, int len )
{
	u16 crc;
	u8 *cmd, *payload;
	cmd = (buf + BUF_CMD_PLACE);
    	payload = (buf + BUF_PAYLOAD_PLACE);
    	crc = crc_itu_t(0x0000, cmd, 1);
    	crc = crc_itu_t(crc, payload, len - BUF_PAYLOAD_PLACE);
	crc = crc & 0xff;
    	return crc;
}
```



### 83 Command

```c
static void sis_make_83_buffer( uint8_t *buf, unsigned int addr, int pack_num )
{
	int len = CMD_83_BYTE;
	/*
	*buf = INT_OUT_REPORT;
	*(buf + 1) = (len - 1) & 0xff;
	*(buf + 2) = 0x83;
	*(buf + 4) = addr & 0xff;
	*(buf + 5) = (addr >> 8) & 0xff;
	*(buf + 6) = (addr >> 16) & 0xff;
	*(buf + 7) = (addr >> 24) & 0xff;
	*(buf + 8) = pack_num & 0xff;
	*(buf + 9) = (pack_num >> 8) & 0xff;*/

	*buf = 0x40;
	*(buf + 1) = 0x01
	*(buf + 2) = 0x0c;
	*(buf + 3) = 0x00;
	*(buf + 4) = INT_OUT_REPORT;
	*(buf + 5) = 0x00;
	*(buf + 6) = 0x83;
	
	*(buf + 8) = addr & 0xff;
	*(buf + 9) = (addr >> 8) & 0xff;
	*(buf + 10) = (addr >> 16) & 0xff;
	*(buf + 11) = (addr >> 24) & 0xff;
	*(buf + 12) = pack_num & 0xff;
	*(buf + 13) = (pack_num >> 8) & 0xff;
	
	// crc
	*(buf + BUF_CRC_PLACE) = sis_calculate_output_crc( buf, len );

}
```



### 84 Command

```C
static void sis_make_84_buffer( uint8_t *buf, const u8 *val, int count )
{
	int i, j, k;
	int len = BUF_PAYLOAD_PLACE + count;
	/*
	*buf = INT_OUT_REPORT;
	*(buf + 1) = (len - 1) & 0xff; // byte_count field: package length - 1
	*(buf + 2) = 0x84;*/

	*buf = 0x40;
	*(buf + 1) = 0x01;
	*(buf + 2) = (len - 2) & 0xff;
	*(buf + 3) = 0x00;
	*(buf + 4) = INT_OUT_REPORT;
	*(buf + 5) = 0x00;
	*(buf + 6) = 0x84;
	
	for (i = 0; i < count; i+=4) {
		k = i + 3;
		for (j = i; j < i+4; j++) {
			*(buf + BUF_PAYLOAD_PLACE + j) = *(val + k) & 0xff;
			k--;
		}
	}
	if ((count % 4) != 0) {
		k = count / 4 * 4;
		for (j = count-1; j >= (count/4*4); j--) {
			*(buf + BUF_PAYLOAD_PLACE + j) = *(val + k) & 0xff;
			k++;
		}
	}
	// crc
	*(buf + BUF_CRC_PLACE) = sis_calculate_output_crc( buf, len );

}
```



### Change Mode

```C
bool sis_switch_to_cmd_mode(struct i2c_client *client)
{
	int ret = -1;
	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t sis817_cmd_active[10] 	= {0x40, 0x01, 0x08, 0x00, 0x09, 
		0x00, 0x85, 0x0d, 0x51, 0x09};
	uint8_t sis817_cmd_enable_diagnosis[10] 	= {0x40, 0x01, 0x08, 
		0x00, 0x09, 0x00, 0x85, 0x5c, 0x21, 0x01};
	
	
	

	//Send 85 CMD - PWR_CMD_ACTIVE
	ret = sis_command_for_write(client, sizeof(sis817_cmd_active), sis817_cmd_active);
	if(ret < 0){
		pr_err(KERN_ERR "SiS SEND Switch CMD Faile - 85(PWR_CMD_ACTIVE)\n");
		return -1;
	}
	
	ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
	if(ret < 0){
		pr_err(KERN_ERR "SiS READ Switch CMD Faile - 85(PWR_CMD_ACTIVE)\n");
		return -1;
	}
	
	if((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
		pr_err(KERN_ERR "SiS SEND Switch CMD Return NACK - 85(PWR_CMD_ACTIVE)\n");
		return -1;
	}else if((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
		pr_err(KERN_ERR "SiS SEND Switch CMD Return Unknow- 85(PWR_CMD_ACTIVE)\n");
		return -1;
	}
	
	msleep(100);
	memset(tmpbuf, 0, sizeof(tmpbuf));
	
	//Send 85 CMD - ENABLE_DIAGNOSIS_MODE
	ret = sis_command_for_write(client, sizeof(sis817_cmd_enable_diagnosis), sis817_cmd_enable_diagnosis);
	if(ret < 0){
		pr_err(KERN_ERR "SiS SEND Switch CMD Faile - 85(ENABLE_DIAGNOSIS_MODE)\n");
		return -1;
	}
	
	ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
	if(ret < 0){
		pr_err(KERN_ERR "SiS READ Switch CMD Faile - 85(ENABLE_DIAGNOSIS_MODE)\n");
		return -1;
	}
	
	if((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
		pr_err(KERN_ERR "SiS SEND Switch CMD Return NACK - 85(ENABLE_DIAGNOSIS_MODE)\n");
		return -1;
	}else if((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
		pr_err(KERN_ERR "SiS SEND Switch CMD Return Unknow- 85(ENABLE_DIAGNOSIS_MODE)\n");
		return -1;
	}
	
	msleep(50);
	return 0;	

}
```

```C
bool sis_switch_to_work_mode(struct i2c_client *client)
{
	int ret = -1;
	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t sis817_cmd_fwctrl[10] 	= {0x40, 0x01, 0x08, 0x00, 0x09, 
		0x00, 0x85, 0x3c, 0x50, 0x09};
	uint8_t sis817_cmd_disable_diagnosis[10] 	= {0x40, 0x01, 0x08, 
		0x00, 0x09, 0x00, 0x85, 0x6d, 0x20, 0x01};


	//Send 85 CMD - PWR_CMD_FW_CTRL
	ret = sis_command_for_write(client, sizeof(sis817_cmd_fwctrl), sis817_cmd_fwctrl);
	if(ret < 0){
		pr_err(KERN_ERR "SiS SEND Switch CMD Faile - 85(PWR_CMD_FW_CTRL)\n");
		return -1;
	}
	
	ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
	if(ret < 0){
		pr_err(KERN_ERR "SiS READ Switch CMD Faile - 85(PWR_CMD_FW_CTRL)\n");
		return -1;
	}
	
	if((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
		pr_err(KERN_ERR "SiS SEND Switch CMD Return NACK - 85(PWR_CMD_FW_CTRL)\n");
		return -1;
	}else if((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
		pr_err(KERN_ERR "SiS SEND Switch CMD Return Unknow- 85(PWR_CMD_FW_CTRL)\n");
		return -1;
	}
	
	memset(tmpbuf, 0, sizeof(tmpbuf));
	
	//Send 85 CMD - DISABLE_DIAGNOSIS_MODE
	ret = sis_command_for_write(client, sizeof(sis817_cmd_disable_diagnosis), sis817_cmd_disable_diagnosis);
	if(ret < 0){
		pr_err(KERN_ERR "SiS SEND Switch CMD Faile - 85(DISABLE_DIAGNOSIS_MODE)\n");
		return -1;
	}
	
	ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
	if(ret < 0){
		pr_err(KERN_ERR "SiS READ Switch CMD Faile - 85(DISABLE_DIAGNOSIS_MODE)\n");
		return -1;
	}
	
	if((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
		pr_err(KERN_ERR "SiS SEND Switch CMD Return NACK - 85(DISABLE_DIAGNOSIS_MODE)\n");
		return -1;
	}else if((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
		pr_err(KERN_ERR "SiS SEND Switch CMD Return Unknow- 85(DISABLE_DIAGNOSIS_MODE)\n");
		return -1;
	}
	
	msleep(50);
	return 0;

}
```



### Check Chip Status

#### Return: Ture is chip on the work function, else is chip not ready.

```C
bool sis_check_fw_ready(struct i2c_client *client)
{


int ret = 0;
int check_num = 10;
uint8_t tmpbuf[MAX_BYTE] = {0};
uint8_t sis817_cmd_check_ready[14] = {0x40, 0x01, 0x0c, 0x00, 0x09, 
	0x00, 0x86, 0x00, 0x00, 0x00, 0x00, 0x50, 0x34, 0x00};

sis817_cmd_check_ready[BUF_CRC_PLACE] = (0xFF & cal_crc_with_cmd(sis817_cmd_check_ready, 8, 13, 0x86));


if(!sis_switch_to_cmd_mode(client)){
	pr_err(KERN_ERR "SiS Switch to CMD mode error.\n");
	return false;
}

while(check_num--){
	pr_err(KERN_ERR "SiS Check FW Ready.\n");
	ret = sis_command_for_write(client, sizeof(sis817_cmd_check_ready), sis817_cmd_check_ready);
	if(ret < 0){
		pr_err(KERN_ERR "SiS SEND Check FW Ready CMD Faile - 86\n");
	}
	ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
	if((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
		pr_err(KERN_ERR "SiS SEND Check FW Ready CMD Return NACK\n");
	}else if((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
		pr_err(KERN_ERR "SiS SEND Check FW Ready CMD Return Unknow\n");
	}else{
		if(tmpbuf[9] == 1){
			pr_err(KERN_ERR "SiS FW READY.\n");
			break;
		}
	}
	pr_err(KERN_ERR "SiS CHECK FW READY - Retry:%d.\n", (10-check_num));	
	msleep(50);
}

if(!sis_switch_to_work_mode(client)){
	pr_err(KERN_ERR "SiS Switch to Work mode error.\n");
	return false;
}

if(check_num == 0) return false;
return true;

}
```



### Change Chip to Power Mode

> Use this function to change chip power mode. 
>
> mode:POWER_MODE_FWCTRL, power control by FW.
>
> POWER_MODE_ACTIVE, chip always work on time.
>
> POWER_MODE_SLEEP,  chip on the sleep mode.
>
> Return:Ture is change power mode success.

```C
bool sis_change_fw_mode(struct i2c_client *client, enum SIS_817_POWER_MODE mode)
{
int ret = -1;
uint8_t tmpbuf[MAX_BYTE] = {0};
uint8_t sis817_cmd_fwctrl[10] 	= {0x40, 0x01, 0x08, 0x00, 0x09, 0x00, 0x85, 0x3c, 0x50, 0x09};
uint8_t sis817_cmd_active[10] 	= {0x40, 0x01, 0x08, 0x00, 0x09, 0x00, 0x85, 0x0d, 0x51, 0x09};
uint8_t sis817_cmd_sleep[10] 	= {0x40, 0x01, 0x08, 0x00, 0x09, 0x00, 0x85, 0x5e, 0x52, 0x09};


switch(mode)
{
case POWER_MODE_FWCTRL:
	ret = sis_command_for_write(client, sizeof(sis817_cmd_fwctrl), sis817_cmd_fwctrl);
	break;
case POWER_MODE_ACTIVE:
	ret = sis_command_for_write(client, sizeof(sis817_cmd_active), sis817_cmd_active);
	break;
case POWER_MODE_SLEEP:
	ret = sis_command_for_write(client, sizeof(sis817_cmd_sleep), sis817_cmd_sleep);
	break;
default:
	return -1;
	break;
}

if(ret < 0){
	pr_err(KERN_ERR "SiS SEND Power CMD Faile - 85\n");
	return -1;
}

ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
if(ret < 0){
	pr_err(KERN_ERR "SiS READ Power CMD Faile - 85\n");
	return -1;
}

if((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
	pr_err(KERN_ERR "SiS SEND Power CMD Return NACK - 85\n");
	return -1;
}else if((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
	pr_err(KERN_ERR "SiS SEND Power CMD Return Unknow- 85\n");
	return -1;
}

msleep(100);

return 0;
 }
```



### Get Chip status

> Use this function to . 
> Return:-1 is get firmware work status error.
> POWER_MODE_FWCTRL, power control by FW.
> POWER_MODE_ACTIVE, chip always work on time.
> POWER_MODE_SLEEP,  chip on the sleep mode.

``` C
enum SIS_817_POWER_MODE sis_get_fw_mode(struct i2c_client *client)
{
	int ret;
	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t sis817_cmd_check_power_mode[14] = {0x40, 0x01, 0x0c, 0x00, 
		0x09, 0x00, 0x86, 0x00, 0x00, 0x00, 0x00, 0x50, 0x34, 0x00};

	pr_err("SiS Get FW Mode.\n");	
	sis817_cmd_check_power_mode[BUF_CRC_PLACE] = (0xFF & cal_crc_with_cmd(sis817_cmd_check_power_mode, 8, 13, 0x86));
	
	ret = sis_command_for_write(client, sizeof(sis817_cmd_check_power_mode), sis817_cmd_check_power_mode);
	if(ret < 0){
		pr_err(KERN_ERR "SiS SEND Get FW Mode CMD Faile - 86\n");
	}else{
		ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
		if(ret < 0){
			pr_err(KERN_ERR "SiS READ Get FW Mode CMD Faile - 86\n");
		}else{
			if((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
				pr_err(KERN_ERR "SiS SEND Get FW Mode CMD Return NACK\n");
			}else if((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
				pr_err(KERN_ERR "SiS SEND Get FW Mode CMD Return Unknow\n");
				PrintBuffer(0, sizeof(tmpbuf), tmpbuf);
			}
		}
	}
	
	switch(tmpbuf[10])
	{
	case POWER_MODE_FWCTRL:
		return POWER_MODE_FWCTRL;
	case POWER_MODE_ACTIVE:
		return POWER_MODE_ACTIVE;
	case POWER_MODE_SLEEP:
		return POWER_MODE_SLEEP;
	default:
		break;
	}

	return -1;
}
```



### Software Reset

```C
  void sis_fw_softreset(struct i2c_client *client)
  {

	int ret = 0;
	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t sis817_cmd_reset[8] = {0x40, 0x01, 0x06, 0x00, 0x09, 0x00, 0x82, 0x00};

	sis817_cmd_reset[BUF_CRC_PLACE] = (0xFF & cal_crc(sis817_cmd_reset, 6, 6));

	pr_err(KERN_ERR "SiS Software Reset.\n");
	if(!sis_switch_to_cmd_mode(client)){
		pr_err(KERN_ERR "SiS Switch to CMD mode error.\n");
		return;
	}
	
	ret = sis_command_for_write(client, sizeof(sis817_cmd_reset), sis817_cmd_reset);
	if(ret < 0){
		pr_err(KERN_ERR "SiS SEND Reset CMD Faile - 82\n");
	}
	ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
	if((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
		pr_err(KERN_ERR "SiS SEND Reset CMD Return NACK - 85(DISABLE_DIAGNOSIS_MODE)\n");
	}else if((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
		pr_err(KERN_ERR "SiS SEND Reset CMD Return Unknow- 85(DISABLE_DIAGNOSIS_MODE)\n");
	}	
	msleep(2000);
  }
```



### Boot Loader Flags

```c
static bool sis_get_bootflag(struct i2c_client *client, u32 *bootflag)
{
	int ret = 0;
	//int i=0;
	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t sis_cmd_get_bootflag[CMD_86_BYTE] = {0x40, 0x01, 0x0c, 0x00, 0x09, 
		0x00, 0x86, 0x00, 0xf0, 0xef, 0x01, 0xa0, 0x34, 0x00};
	sis_cmd_get_bootflag[BUF_CRC_PLACE] = sis_calculate_output_crc(sis_cmd_get_bootflag, CMD_86_BYTE );


// write
ret = sis_command_for_write(client, sizeof(sis_cmd_get_bootflag), sis_cmd_get_bootflag);
if (ret < 0) {
	pr_err(KERN_ERR "sis SEND Get Bootloader ID CMD Failed - 86 %d\n", ret);
	return -1;
}

// read	
ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);

//pr_err("sis_get_bootflag read data:\n");
//PrintBuffer(0, MAX_BYTE, tmpbuf);

*bootflag = (tmpbuf[8] << 24) | (tmpbuf[9] << 16) | (tmpbuf[10] << 8) | (tmpbuf[11]);
return 0;

}

static bool sis_get_bootloader_id_crc(struct i2c_client *client, u32 *bootloader_version, u32 *bootloader_crc)
{
	int ret = 0;
	//int i=0;
	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t sis_cmd_get_bootloader_id_crc[CMD_86_BYTE] = {0x40, 0x01, 0x0c, 0x00, 0x09, 
		0x00, 0x86, 0x00, 0x30, 0x02, 0x00, 0xa0, 0x34, 0x00};
	sis_cmd_get_bootloader_id_crc[BUF_CRC_PLACE] = sis_calculate_output_crc(sis_cmd_get_bootloader_id_crc, CMD_86_BYTE );


// write
ret = sis_command_for_write(client, sizeof(sis_cmd_get_bootloader_id_crc), sis_cmd_get_bootloader_id_crc);
if (ret < 0) {
	pr_err(KERN_ERR "sis SEND Get Bootloader ID CMD Failed - 86 %d\n", ret);
	return -1;
}

// read	
ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);

//pr_err("sis_get_bootloader_id_crc read data:\n");
//PrintBuffer(0, MAX_BYTE, tmpbuf);

*bootloader_version = (tmpbuf[8] << 24) | (tmpbuf[9] << 16) | (tmpbuf[10] << 8) | (tmpbuf[11]);
*bootloader_crc = (tmpbuf[12] << 24) | (tmpbuf[13] << 16) | (tmpbuf[14] << 8) | (tmpbuf[15]);
return 0;

}
```



### Get Firmware ID 

```c
static bool sis_get_fw_id(struct i2c_client *client, u16 *fw_version)
{
	int ret = 0;
	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t sis_cmd_get_FW_ID[CMD_86_BYTE] = {0x40, 0x01, 0x0c, 0x00, 0x09, 
		0x00, 0x86, 0x00, 0x00, 0x40, 0x00, 0xa0, 0x34, 0x00};
	sis_cmd_get_FW_ID[BUF_CRC_PLACE] = sis_calculate_output_crc( sis_cmd_get_FW_ID, CMD_86_BYTE );


// write
ret = sis_command_for_write(client, sizeof(sis_cmd_get_FW_ID), sis_cmd_get_FW_ID);
if (ret < 0) {
	pr_err(KERN_ERR "sis SEND Get FW ID CMD Failed - 86 %d\n", ret);
	return -1;
}

// read	
ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);

/*pr_err("sis_get_fw_info read data:\n");
PrintBuffer(0, MAX_BYTE, tmpbuf);*/

*fw_version = (tmpbuf[22] << 8) | (tmpbuf[23]);
return 0;

}
```



### Read / Write Firmware Information

```c
static bool sis_get_fw_info(struct i2c_client *client, u8 *chip_id, u32 *tp_size, u32 *tp_vendor_id, u16 *task_id, u8 *chip_type, u16 *fw_version)
{
	int ret = 0;
	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t sis_cmd_get_FW_INFO[CMD_86_BYTE] = {0x40, 0x01, 0x0c, 0x00, 0x09, 
		0x00, 0x86, 0x00, 0x00, 0x40, 0x00, 0xa0, 0x34, 0x00};
	sis_cmd_get_FW_INFO[BUF_CRC_PLACE] = sis_calculate_output_crc( sis_cmd_get_FW_INFO, CMD_86_BYTE );


// write
ret = sis_command_for_write(client, sizeof(sis_cmd_get_FW_INFO), sis_cmd_get_FW_INFO);
if (ret < 0) {
	pr_err(KERN_ERR "sis SEND Get FW ID CMD Failed - 86 %d\n", ret);
	return -1;
}

// read	
ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);

/*pr_err("sis_get_fw_info read data:\n");
PrintBuffer(0, MAX_BYTE, tmpbuf);*/

*chip_id = tmpbuf[10];
*tp_size = (tmpbuf[11] << 16) | (tmpbuf[12] << 8) | (tmpbuf[13]);
*tp_vendor_id = (tmpbuf[14] << 24) | (tmpbuf[15] << 16) | (tmpbuf[16] << 8) | (tmpbuf[17]);
*task_id = (tmpbuf[18] << 8) | (tmpbuf[19]);
*chip_type = tmpbuf[21];
*fw_version = (tmpbuf[22] << 8) | (tmpbuf[23]);
return 0;

}

static bool sis_write_fw_info(unsigned int addr, int pack_num, struct i2c_client *client)
{
	int ret = 0;
	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t sis817_cmd_83[CMD_83_BYTE] = {0};
	sis_make_83_buffer(sis817_cmd_83, addr, pack_num);
	//pr_err("sis_write_fw_info cmd_83:\n"); 
	//PrintBuffer(0, CMD_83_BYTE, sis817_cmd_83);
	// write
	ret = sis_command_for_write(client, sizeof(sis817_cmd_83), sis817_cmd_83);
	if (ret < 0) {
		pr_err(KERN_ERR "sis SEND write CMD Failed - 83(WRI_FW_DATA_INFO) %d\n", ret);
		return -1;
	}
	// read	
	ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
	if (ret < 0) {
		pr_err(KERN_ERR "sis READ write CMD Failed - 83(WRI_FW_DATA_INFO) %d\n", ret);
		return -1;
	}
	//pr_err("sis_write_fw_info tmpbuf:\n"); 
	//PrintBuffer(0, 14, tmpbuf);
	// Check ACK
	if ((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
		pr_err(KERN_ERR "sis READ write CMD Return NACK - 83(WRI_FW_DATA_INFO)\n");
		return -1;
	} else if ((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
		pr_err(KERN_ERR "sis READ write CMD Return Unknown- 83(WRI_FW_DATA_INFO)\n");
		return -1;
	}
	return 0;
}

static bool sis_write_fw_payload(const u8 *val, unsigned int count, struct i2c_client *client)
{
	int ret = 0;
	int len = BUF_PAYLOAD_PLACE + count;
	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t *sis817_cmd_84 = kzalloc(len, GFP_KERNEL);


if (!sis817_cmd_84) {
	pr_err(KERN_ERR "sis alloc buffer error\n");
	return -1;	
}
sis_make_84_buffer(sis817_cmd_84, val, count);
//pr_err("sis_command_for_write\n");
//PrintBuffer(0, 64, sis817_cmd_84);
// write
ret = sis_command_for_write(client, len, sis817_cmd_84);
if (ret < 0) {
	pr_err(KERN_ERR "sis SEND write CMD Failed - 84(WRI_FW_DATA_PAYL) %d\n", ret);
	return -1;
}
// read	
ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
if (ret < 0) {
	pr_err(KERN_ERR "sis READ write CMD Failed - 84(WRI_FW_DATA_PAYL) %d\n", ret);
	return -1;
}
//pr_err("sis_command_for_read\n");
//PrintBuffer(0, 10, tmpbuf);
// Check ACK
if ((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
	pr_err(KERN_ERR "sis READ write CMD Return NACK - 84(WRI_FW_DATA_PAYL)\n");
	return -1;
} else if ((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
	pr_err(KERN_ERR "sis READ write CMD Return Unknown- 84(WRI_FW_DATA_PAYL)\n");
	return -1;
}
kfree(sis817_cmd_84);
return 0;

}
```



### Flash ROM

```C
static bool sis_flash_rom(struct i2c_client *client)
{
	int ret = 0;
    	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t sis_cmd_81[CMD_81_BYTE] = {0x40, 0x01, 0x06, 0x00, 0x09, 0x00, 0x81, 0x00};


sis_cmd_81[BUF_CRC_PLACE] = sis_calculate_output_crc( sis_cmd_81, CMD_81_BYTE );

//PrintBuffer(0, 4, sis817_cmd_81);
// write
ret = sis_command_for_write(client, sizeof(sis_cmd_81), sis_cmd_81);
if (ret < 0) {
	pr_err(KERN_ERR "sis SEND flash CMD Failed - 81(FLASH_ROM) %d\n", ret);
	return -1;
}
msleep(2000);
// read	
ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
if (ret < 0) {
	pr_err(KERN_ERR "sis READ flash CMD Failed - 81(FLASH_ROM) %d\n", ret);
	return -1;
}
PrintBuffer(0, 10, tmpbuf);
// Check ACK
if ((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
	pr_err(KERN_ERR "sis READ flash CMD Return NACK - 81(FLASH_ROM)\n");
	return -1;
} else if ((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
	pr_err(KERN_ERR "sis READ flash CMD Return Unknown- 81(FLASH_ROM)\n");
	return -1;
}
return 0;

}

static bool sis_clear_bootflag(struct i2c_client *client)
{
	int ret, retry, i;
	unsigned int pack_num = 0;
	unsigned int count_84 = 0, size_84 = 0;
	u8 *tmpbuf;
	tmpbuf = kzalloc(64, GFP_KERNEL);
	pack_num = (0x1000 / PACK_SIZE) + (((0x1000 % PACK_SIZE) == 0)? 0 : 1);
	for (retry = 0; retry < 3; retry++) {
		//ret = sis_write_fw_info(0x0000efcc, 1, id);
		pr_err("Write to addr = 0001e000 pack_num=%d \n", pack_num);
		ret = sis_write_fw_info(0x0001e000, pack_num, client);
		if (ret) {
			pr_err(KERN_ERR "sis Write FW info (0x83) error.\n");
			continue;
		}
		count_84 = 0x1e000;
		for (i = 0; i < pack_num; i++) {
			size_84 = (0x1f000 > (count_84 + PACK_SIZE))?
				PACK_SIZE : (0x1f000 - count_84);
			ret = sis_write_fw_payload(tmpbuf, size_84, client);
			if (ret)
				break;
			count_84 += size_84;
		}
		if (ret) {
			pr_err(KERN_ERR "sis Write FW payload (0x84) error\n");
			continue;
		}
		//msleep(1000);
		ret = sis_flash_rom(client);
		if (ret) {
			pr_err(KERN_ERR "sis Flash ROM (0x81) error.\n");
			continue;
		} else {
			pr_err("sis update block success.\n");
			break;
		}	
	}
	kfree(tmpbuf);
	if (ret < 0) {
		pr_err(KERN_ERR "Retry timeout\n");
		return -1;
	}
	return 0;
}

static bool sis_update_block(uint8_t *data, unsigned int addr, unsigned int count, 
			      struct i2c_client *client)
{
	int i, ret, block_retry;
	unsigned int end = addr + count;
	unsigned int count_83 = 0, size_83 = 0; // count_83: address, size_83: length
	unsigned int count_84 = 0, size_84 = 0; // count_84: address, size_84: length
	unsigned int pack_num = 0;
	

count_83 = addr;
while (count_83 < end) {
	size_83 = end > (count_83 + RAM_SIZE)? RAM_SIZE : (end - count_83);
	//pr_err("sis_update_block size83 = %d, count_83 = %d\n", size_83, count_83);
	pack_num = (size_83 / PACK_SIZE) + (((size_83 % PACK_SIZE) == 0)? 0 : 1);
	for (block_retry = 0; block_retry < 3; block_retry++) {
		pr_err("Write to addr = %08x pack_num=%d \n", count_83, pack_num);
		ret = sis_write_fw_info(count_83, pack_num, client);
		if (ret) {
			pr_err(KERN_ERR "sis Write FW info (0x83) error.\n");
			continue;
		}
		count_84 = count_83;
		for (i = 0; i < pack_num; i++) {
			size_84 = (count_83 + size_83) > (count_84 + PACK_SIZE)? 
				PACK_SIZE : (count_83 + size_83 - count_84);
			//pr_err("sis_update_block size84 = %d, count_84 = %d\n", size_84, count_84);
			ret = sis_write_fw_payload(data + count_84, size_84, client);
			if (ret)
				break;
			count_84 += size_84;
		}
		if (ret) {
			pr_err(KERN_ERR "sis Write FW payload (0x84) error.\n");
			continue;
		}
		//msleep(1000);
		ret = sis_flash_rom(client);
		if (ret) {
			pr_err(KERN_ERR "sis Flash ROM (0x81) error.\n");
			continue;
		} else {
			pr_err("sis update block success.\n");
			break;
		}	
	}
	if (ret < 0) {
		pr_err(KERN_ERR "Retry timeout\n");
		return -1;
	}
	count_83 += size_83;
	if (count_83 == count_84) {
		pr_err("sis count_83 == count_84.\n");
	}
}
return 0;

}
```



### Reset Firmware

```C
static bool sis_reset_cmd(struct i2c_client *client )
{
	int ret = 0;
	uint8_t tmpbuf[MAX_BYTE] = {0};
	uint8_t sis_cmd_82[CMD_82_BYTE] = {0x40, 0x01, 0x06, 0x00, 0x09, 0x00, 0x82, 0x00};


sis_cmd_82[BUF_CRC_PLACE] = sis_calculate_output_crc(sis_cmd_82, CMD_82_BYTE);

// write
ret = sis_command_for_write(client, sizeof(sis_cmd_82), sis_cmd_82);
if (ret < 0) {
	pr_err(KERN_ERR "sis SEND reset CMD Failed - 82(RESET) %d\n", ret);
	return -1;
}
// read	
ret = sis_command_for_read(client, sizeof(tmpbuf), tmpbuf);
if (ret < 0) {
	pr_err(KERN_ERR "sis READ reset CMD Failed - 82(RESET) %d\n", ret);
	return -1;
}

PrintBuffer(0, 10, tmpbuf);

// Check ACK
if ((tmpbuf[BUF_ACK_PLACE_L] == BUF_NACK_L) && (tmpbuf[BUF_ACK_PLACE_H] == BUF_NACK_H)){
	pr_err(KERN_ERR "sis READ reset CMD Return NACK - 82(RESET)\n");
	return -1;
} else if ((tmpbuf[BUF_ACK_PLACE_L] != BUF_ACK_L) || (tmpbuf[BUF_ACK_PLACE_H] != BUF_ACK_H)){
	pr_err(KERN_ERR "sis READ reset CMD Return Unknown- 82(RESET)\n");
	return -1;
}
pr_err("sis reset success.\n");
return 0;

}
```



### Update Firmware

```C
static bool sis_update_fw( struct i2c_client *client, uint8_t *fn, bool update_boot)
{
	//struct device *dev = &client->dev;
	int ret = 0;
	//struct sis_ts_data *ts_data = dev_get_drvdata(client);
	//const struct firmware *fw = NULL;
	

pr_err(KERN_ERR "sis update FW start.\n");
/* Update FW */
/* (1) Clear boot flag */
ret = sis_clear_bootflag(client);
if (ret) {
	pr_err(KERN_ERR "sis Update fw fail at (1).");
	goto release_firmware;
}

/* (2) Update main code 1 */
ret = sis_update_block(fn, 0x00007000, 0x00016000, client);
if (ret) {
	pr_err(KERN_ERR "sis Update fw fail at (2).");
	goto release_firmware;
}

/* (3) Update main code 2 */
ret = sis_update_block(fn, 0x00006000, 0x00001000, client);
if (ret) {
	pr_err(KERN_ERR "sis Update fw fail at (3).");
	goto release_firmware;
}

/* (4) Update fwinfo, regmem, defmem, THQAmem, hidDevDesc, hidRptDesc */
ret = sis_update_block(fn, 0x00004000, 0x00002000, client);
if (ret) {
	pr_err(KERN_ERR "sis Update fw fail at (4).");
	goto release_firmware;
}

if (update_boot) {
	/* (5) Update boot code */
	ret = sis_update_block(fn, 0x00000000, 0x00004000, client);
	if (ret) {
		pr_err(KERN_ERR "sis Update fw fail at (5).");
		goto release_firmware;
	}
}

/* (6) Update rodata */
ret = sis_update_block(fn, 0x0001d000, 0x00002000, client);
if (ret) {
	pr_err(KERN_ERR "sis Update fw fail at (6).");
	goto release_firmware;
}

/* Reset */
ret = sis_reset_cmd(client);
goto release_firmware;

release_firmware:
	//release_firmware(fw);//for request_firmware()
	pr_err("sis release FW.\n");
	if (ret < 0) 
	    return -1;


return 0;

}
```



# Probe in Linux I2C Driver

> Not necessary.

```c
static int sis_ts_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	u8 chip_id = 0x00;
	u8 bin_chip_id = 0x00;
	u32 tp_size = 0x00000000;
	u32 bin_tp_size = 0x00000000;
	u32 tp_vendor_id = 0x00000000;
	u32 bin_tp_vendor_id = 0x00000000;
	u16 task_id = 0x0000;
	u16 bin_task_id = 0x0000;
	u8 chip_type = 0x00;
	u8 bin_chip_type = 0x00;
	u16 fw_version = 0x0000;
	u16 bin_fw_version = 0x0000;
	u32 bootloader_version = 0x00000000;
	u32 bin_bootloader_version = 0x00000000;
	u32 bootloader_crc_version = 0x00000000;
	u32 bin_bootloader_crc_version = 0x00000000;
	u32 bootflag = 0x00000000;
	u32 bin_bootflag = 0x00000000;
	bool update_boot = false;
	bool force_update = false;


msleep(2000);
if (sis_switch_to_cmd_mode(client)) {
	pr_err(KERN_ERR "sis Switch to CMD mode error.\n");
	goto end;
}

/* check FW information with FW ID */
ret = sis_get_fw_info(client, &chip_id, &tp_size, &tp_vendor_id, &task_id, &chip_type, &fw_version);

if (ret)
	pr_err(KERN_ERR "sis get fw info failed %d\n", ret);

//firmware_id = fw_version;

//chip id
bin_chip_id = sis_fw_data[0x4002];
pr_err("sis chip id = %02x, bin = %02x\n", chip_id, bin_chip_id);	

//tp vendor id
bin_tp_vendor_id = (sis_fw_data[0x4006] << 24) | (sis_fw_data[0x4007] << 16) | (sis_fw_data[0x4008] << 8) | (sis_fw_data[0x4009]);
pr_err("sis tp vendor id = %08x, bin = %08x\n", tp_vendor_id, bin_tp_vendor_id);

//task id
bin_task_id = (sis_fw_data[0x400a] << 8) | (sis_fw_data[0x400b]);
pr_err("sis task id = %04x, bin = %04x\n", task_id, bin_task_id);	

//0x400c reserved

//chip type
bin_chip_type = sis_fw_data[0x400d];
pr_err("sis chip type = %02x, bin = %02x\n", chip_type, bin_chip_type);	

//fw version
bin_fw_version = (sis_fw_data[0x400e] << 8) | (sis_fw_data[0x400f]);
pr_err("sis fw version = %04x, bin = %04x\n", fw_version, bin_fw_version);

//check fw info
if ( (chip_id != bin_chip_id) || (tp_size != bin_tp_size) || (tp_vendor_id != bin_tp_vendor_id) || (task_id != bin_task_id) || (chip_type != bin_chip_type) ) {
	pr_err(KERN_ERR "fw info not match, stop update fw.");
	goto work_mode;
}

msleep(2000);

/* check BootFlag */
ret = sis_get_bootflag(client, &bootflag);
if (ret) {
	pr_err(KERN_ERR "sis get bootflag failed %d\n", ret);
}

bin_bootflag = (sis_fw_data[0x1eff0] << 24) | (sis_fw_data[0x1eff1] << 16) | (sis_fw_data[0x1eff2] << 8) | (sis_fw_data[0x1eff3]);
pr_err("sis bootflag = %08x, bin = %08x\n", bootflag, bin_bootflag);	

if (bin_bootflag != SIS_BOOTFLAG_P810) {
	pr_err("bin file broken, stop update fw.\n");
	goto work_mode;
}

if (bootflag != SIS_BOOTFLAG_P810) {
	pr_err("fw broken, force update fw.\n");
	force_update = true;
}

msleep(2000);

/* Compare BootloaderID and Bootloader CRC */
ret = sis_get_bootloader_id_crc(client, &bootloader_version, &bootloader_crc_version);
if (ret) {
	pr_err(KERN_ERR "sis get bootloader id or crc failed %d\n", ret);
}

sis_switch_irq(1);
msleep(200);

sis_switch_irq(0);
msleep(2000);

//bootloader id
bin_bootloader_version = (sis_fw_data[0x230] << 24) | (sis_fw_data[0x231] << 16) | (sis_fw_data[0x232] << 8) | (sis_fw_data[0x233]);
pr_err("sis bootloader id = %08x, bin = %08x\n", bootloader_version, bin_bootloader_version);

//bootloader crc
bin_bootloader_crc_version = (sis_fw_data[0x234] << 24) | (sis_fw_data[0x235] << 16) | (sis_fw_data[0x236] << 8) | (sis_fw_data[0x237]);
pr_err("sis bootloader crc = %08x, bin = %08x\n", bootloader_crc_version, bin_bootloader_crc_version);

if ((bootloader_version != bin_bootloader_version) && (bootloader_crc_version != bin_bootloader_crc_version)) {
	update_boot = true;
	pr_err("bootloader changed. update_boot flag on.\n");
}

//bin_fw_version = 0xab00; //test for update fw
//update_boot = true; //test for update bootloader

if ((bin_fw_version & 0xff00) == 0xab00) {
	force_update = true;
}

if (update_fw_initprobe == false) {
	update_fw_initprobe = true;
	if (((bin_fw_version > fw_version) && (bin_fw_version < 0xab00))
		|| force_update == true)
	{
		//Special Update Flag : 0x9999: update by Driver
		sis_fw_data[0x4000] = 0x99;
		sis_fw_data[0x4001] = 0x99;

		ret = sis_update_fw(ts->client, sis_fw_data, update_boot);
		pr_err("update_fw_initprobe %d\n", update_fw_initprobe);
		//update_fw_initprobe=true;
		if (ret) {
			pr_err(KERN_ERR "sis update fw failed %d\n", ret);
			goto work_mode;
		}
		//firmware_id = bin_fw_version;
	}
	else if (bin_fw_version > 0xabff) {
		pr_err("sis driver update FW :"
				" Unavilable FW version.\n");
		goto work_mode;
	}
		else {			
		pr_err("sis driver update FW :"
			" Current FW version is same or later than bin.\n");
		goto work_mode;
		}
}
else
{	
	pr_err("sis driver update FW :"
		" After AP Update .\n");
	goto work_mode;
}
//sis_gettime();

work_mode:
}
```



# Header File

```c
#include <linux/version.h>

#ifndef _LINUX_SIS_I2C_H
#define _LINUX_SIS_I2C_H

#define SIS_I2C_NAME "sis_i2c_ts"
#define SIS_SLAVE_ADDR					0x5c
#define TIMER_NS						10000000/*10ms*/
#define MAX_FINGERS						10

/* For Android 4.0 */
/* Only for Linux kernel 2.6.34 and later */

/* For standard R/W IO ( SiS firmware application )*/
#define _STD_RW_IO						/*ON/OFF*/
/* Interrupt setting and modes */
#define _I2C_INT_ENABLE					/*ON/OFF*/
#ifdef CONFIG_REG_BY_BOARDINFO
#define GPIO_IRQ				CONFIG_GPIO_INT_PIN_FOR_SIS
#endif


/* IRQ STATUS */
#define IRQ_STATUS_DISABLED				(1<<16)
#define IRQ_STATUS_ENABLED				0x0

/* Resolution mode */
/* Constant value */
#define SIS_MAX_X						4095
#define SIS_MAX_Y						4095

#define ONE_BYTE						1
#define FIVE_BYTE						5
#define EIGHT_BYTE						8
#define SIXTEEN_BYTE					16
#define PACKET_BUFFER_SIZE				128

#ifdef _CHECK_CRC
uint16_t cal_crc(char *cmd, int start, int end);
#endif

#define SIS_CMD_NORMAL					0x0
#define SIS_CMD_SOFTRESET				0x82
#define SIS_CMD_RECALIBRATE				0x87
#define SIS_CMD_POWERMODE				0x90
#define MSK_TOUCHNUM					0x0f
#define MSK_HAS_CRC						0x10
#define MSK_DATAFMT						0xe0
#define MSK_PSTATE						0x0f
#define MSK_PID							0xf0
#define RES_FMT							0x00
#define FIX_FMT							0x40

/* for new i2c format */
#define TOUCHDOWN						0x3
#define TOUCHUP							0x0
#define MAX_BYTE						64
#define	PRESSURE_MAX					255


#define AREA_LENGTH_LONGER				5792
/*Resolution diagonal*/
#define AREA_LENGTH_SHORT				5792
#define AREA_UNIT						(5792/32)



#define FORMAT_MODE						1

#define MSK_NOBTN						0
#define MSK_COMP						1
#define MSK_BACK						2
#define MSK_MENU						4
#define MSK_HOME						8

#define P_BYTECOUNT						0
#define ALL_IN_ONE_PACKAGE				0x10
#ifdef CONFIG_TOUCHSCREEN_SIS_I2C_95XX
#define IS_HIDI2C(x)					(x & 0x1)
#else
#define IS_TOUCH(x)						(x & 0x1)
#define IS_HIDI2C(x)					(x & 0x6)
#endif
#define IS_BTN(x)						(x & 0x4)
#ifdef CONFIG_TOUCHSCREEN_SIS_I2C_95XX
#define IS_AREA(x)						((x >> 4) & 0x1)
#define IS_PRESSURE(x)				    ((x >> 6) & 0x1)
#define IS_SCANTIME(x)			        ((x >> 7) & 0x1)
#else
#define IS_AREA(x)						((x >> 4) & 0x1)
#define IS_PRESSURE(x)				    ((x >> 5) & 0x1)
#define IS_SCANTIME(x)			        ((x >> 6) & 0x1)
#endif
#define NORMAL_LEN_PER_POINT			6
#define AREA_LEN_PER_POINT				2
#define PRESSURE_LEN_PER_POINT			1
/*#define _DEBUG_PACKAGE*/				/* ON/OFF */
/*#define _DEBUG_PACKAGE_WORKFUNC*/		/* ON/OFF */
/*#define _DEBUG_REPORT*/				/* ON/OFF */

/*#define _CHECK_CRC*/					/* ON/OFF */
#ifdef CONFIG_TOUCHSCREEN_SIS_I2C_95XX
#define BUTTON_FORMAT					0x4
#define HIDI2C_FORMAT					0x1
#else
#define TOUCH_FORMAT					0x1
#define BUTTON_FORMAT					0x4
#define HIDI2C_FORMAT					0x6
#endif
#define P_REPORT_ID						2
#define BUTTON_STATE					3
#define BUTTON_KEY_COUNT				16
#define BYTE_BYTECOUNT					2
#define BYTE_COUNT						1
#define BYTE_ReportID					1
#define BYTE_CRC_HIDI2C					0
#define BYTE_CRC_I2C					2
#define BYTE_CRC_BTN					4
#define BYTE_SCANTIME					2
#define NO_TOUCH_BYTECOUNT				0x3

/* SiS i2c error code */
#define SIS_ERR						-1
#define SIS_ERR_EMPTY_PACKET		-2	/* 3 bytes empty packet */
#define SIS_ERR_ACCESS_USER_MEM		-11 /* Access user memory fail */
#define SIS_ERR_ALLOCATE_KERNEL_MEM	-12 /* Allocate memory fail */
#define SIS_ERR_CLIENT				-13 /* Client not created */
#define SIS_ERR_COPY_FROM_USER		-14 /* Copy data from user fail */
#define SIS_ERR_COPY_FROM_KERNEL	-19 /* Copy data from kernel fail */
#define SIS_ERR_TRANSMIT_I2C		-21 /* Transmit error in I2C */

/* TODO */
#define TOUCH_POWER_PIN					0
#define TOUCH_RESET_PIN					1

/* CMD Define */
#define BUF_ACK_PLACE_L					4
#define BUF_ACK_PLACE_H					5
#define BUF_ACK_L						0xEF
#define BUF_ACK_H						0xBE
#define BUF_NACK_L						0xAD
#define BUF_NACK_H						0xDE
#define BUF_CRC_PLACE					7
#define MAX_SLOTS						20

/* working mode */
#define MODE_IS_TOUCH					0
#define MODE_IS_CMD						1
#define MODE_CHANGE						2

#define SIS_UPDATE_FW
#ifdef SIS_UPDATE_FW
	#define SIS_FW_NAME		"sistouch.fw"
	#define RAM_SIZE		8*1024
	#define PACK_SIZE		52
	/* CMD Define */
	#define BUF_ACK_PLACE_L					4
	#define BUF_ACK_PLACE_H					5
	#define BUF_ACK_L						0xEF
	#define BUF_ACK_H						0xBE
	#define BUF_NACK_L						0xAD
	#define BUF_NACK_H						0xDE
	#define BUF_CMD_PLACE	6
	#define BUF_CRC_PLACE	7
	#define BUF_PAYLOAD_PLACE	8
	#define INT_OUT_REPORT		0x09
	#define CMD_81_BYTE		8
	#define CMD_82_BYTE		8
	#define CMD_85_BYTE		10
	#define CMD_83_BYTE		14
	#define CMD_86_BYTE		14
	#define SIS_GENERAL_TIMEOUT	1000
	#define SIS_BOOTFLAG_P810	0x50383130
#endif /* SIS_UPDATE_FW */

#endif /* _LINUX_SIS_I2C_H */

enum SIS_817_POWER_MODE {
	POWER_MODE_FWCTRL = 0x50,
	POWER_MODE_ACTIVE = 0x51,
	POWER_MODE_SLEEP  = 0x52
};

struct sis_i2c_rmi_platform_data {
	int (*power)(int on);	/* Only valid in first array entry */
};

#ifdef SIS_UPDATE_FW

	bool update_fw_initprobe=false;

#ifdef CONFIG_PROC_FS
	u16 firmware_id = 0x0000;
#endif
	static uint8_t sis_fw_data[] = {
        #include SIS_FW_NAME
		};
#endif

static const unsigned short crc16tab[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

struct sis_ts_data {
	int (*power)(int on);
	int use_irq;
	int is_cmd_mode;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct hrtimer timer;
	struct irq_desc *desc;
	struct work_struct work;
	struct mutex mutex_wq;
#ifndef CONFIG_REG_BY_BOARDINFO
	int irq;
#endif
};
```