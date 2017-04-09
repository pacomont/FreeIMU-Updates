// I2Cdev library collection - MPU60X0 I2C device class
// Based on InvenSense MPU-6050 register map document rev. 2.0, 5/19/2011 (RM-MPU-6000A-00)
// 8/24/2011 by Jeff Rowberg <jeff@rowberg.net>
// Updates should (hopefully) always be available at https://github.com/jrowberg/i2cdevlib
//
// Changelog:
//     ... - ongoing debug release

// NOTE: THIS IS ONLY A PARIAL RELEASE. THIS DEVICE CLASS IS CURRENTLY UNDERGOING ACTIVE
// DEVELOPMENT AND IS STILL MISSING SOME IMPORTANT FEATURES. PLEASE KEEP THIS IN MIND IF
// YOU DECIDE TO USE THIS PARTICULAR CODE FOR ANYTHING.

/* ============================================
I2Cdev device library code is placed under the MIT license
Copyright (c) 2011 Jeff Rowberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/

#include "MPU60X0.h"

/** Default constructor, uses default I2C address or default SS Pin if SPI
 * @see MPU60X0_DEFAULT_ADDRESS
 * @see MPU60X0_DEFAULT_SS_PIN
 */
MPU60X0::MPU60X0() {
  devAddr = MPU60X0_DEFAULT_ADDRESS;
}

/** Specific address constructor.
 * @param address I2C address or Slave select Pin (SPI)
 * @see MPU60X0_DEFAULT_ADDRESS
 * @see MPU60X0_DEFAULT_SS_PIN
 * @see MPU60X0_ADDRESS_AD0_LOW
 * @see MPU60X0_ADDRESS_AD0_HIGH
 */
MPU60X0::MPU60X0(bool useSPI, uint8_t address) {
  bSPI = useSPI;
  devAddr = address;
}

/** Power on and prepare for general usage.
 * This will activate the device and take it out of sleep mode (which must be done
 * after start-up). This function also sets both the accelerometer and the gyroscope
 * to their most sensitive settings, namely +/- 2g and +/- 250 degrees/sec, and sets
 * the clock source to use the X Gyro for reference, which is slightly better than
 * the default internal clock source.
 */
void MPU60X0::initialize() {
    // SPI Configuration
	if (bSPI) {
      SPI.begin();
    	pinMode(devAddr, OUTPUT);
    	digitalWrite(devAddr, HIGH);
		reset();
		delay(100);
		switchSPIEnabled(true);
		delay(1);
		setClockSource(MPU60X0_CLOCK_PLL_ZGYRO);
		delay(1);
	} else {
		switchSPIEnabled(false);
		setClockSource(MPU60X0_CLOCK_PLL_XGYRO);
	}
    //setClockSource(MPU60X0_CLOCK_PLL_XGYRO);
	// ArduIMU code states that the z-axis gryo is better so lets give it a try
	// put it SPI test since only APM 2.5 uses SPI
    setFullScaleGyroRange(MPU60X0_GYRO_FS_250);
    setFullScaleAccelRange(MPU60X0_ACCEL_FS_2);
    setSleepEnabled(false); // thanks to Jack Elston for pointing this one out!
}

/** initialize 9250.
 * 
 */
void MPU60X0::initialize9250() {
	//uint8_t data;
    // SPI Configuration
	if (bSPI) {
      SPI.begin();
    	pinMode(devAddr, OUTPUT);
    	digitalWrite(devAddr, HIGH);
		reset();
		delay(100);
		switchSPIEnabled(true);
		delay(1);
	} else {
		switchSPIEnabled(false);
	}
	reset();
	setStandbyDisable();
	setSleepEnabled(false); // thanks to Jack Elston for pointing this one out!
    setClockSource(MPU60X0_CLOCK_PLL_XGYRO);
    setFullScaleGyroRange(MPU60X0_GYRO_FS_2000);
    setFullScaleAccelRange(MPU60X0_ACCEL_FS_2);
	setI2CMasterModeEnabled(false); 
	setI2CBypassEnabled(true);

	//data = 1000 / rate - 1;
	//setRate(data);	
}

void MPU60X0::initialize9250MasterMode(){
	#include "AK8963.h"
	
	uint8_t buff[3];
    uint8_t data[7];
	float _magScaleX, _magScaleY, _magScaleZ;	
	
	//set dev address for magnetometer
	magDevAddr = AK8963_DEFAULT_ADDRESS;
	
    // SPI Configuration
	if (bSPI) {
      SPI.begin();
    	pinMode(devAddr, OUTPUT);
    	digitalWrite(devAddr, HIGH);
		reset();
		delay(100);
		switchSPIEnabled(true);
		delay(1);
	} else {
		Wire.begin(400000);
		switchSPIEnabled(false);
	}
	
	reset();

	setStandbyDisable();
	setSleepEnabled(false);
	
    // select clock source to gyro
    if( !writeRegister(MPU60X0_RA_PWR_MGMT_1, MPU60X0_CLOCK_PLL_XGYRO) ){
        Serial.println("Clock Source Not Set");
    }

    // enable I2C master mode
    if( !writeRegister(MPU60X0_RA_USER_CTRL,I2C_MST_EN) ){
        Serial.println("Master Mode Not Set");
    }

    // set the I2C bus speed to 400 kHz
    if( !writeRegister(MPU60X0_RA_I2C_MST_CTRL,I2C_MST_CLK) ){
        Serial.println("I2C Bus Speed Not Set");
    }

    // set AK8963 to Power Down
    if( !writeAKRegister(AK8963_RA_CNTL1, AK8963_MODE_POWERDOWN) ){
        Serial.println("AK Not Powered Down");
    }

    // reset the MPU9250
    writeRegister(MPU60X0_RA_PWR_MGMT_1, PWR_RESET);

    // wait for MPU-9250 to come back up
    delay(1);

    // reset the AK8963
    writeAKRegister(AK8963_RA_CNTL2, PWR_RESET);

    // select clock source to gyro
    if( !writeRegister(MPU60X0_RA_PWR_MGMT_1, MPU60X0_CLOCK_PLL_XGYRO) ){
        Serial.println("Clock Source Not Set");
    }

    // check the WHO AM I byte, expected value is 0x71 (decimal 113)
	readRegister(MPU60X0_RA_WHO_AM_I, 1,&buff[0]);
    if( buff[0] != 113 ){
        Serial.println("9250 Not Recognized");
    }

    // enable accelerometer and gyro
    if( !writeRegister(MPU60X0_RA_PWR_MGMT_2, SEN_ENABLE) ){
        Serial.println("Accel and Gyro not Enabled");
    }

    /* setup the accel and gyro ranges */
	setFullScaleGyroRange(MPU60X0_GYRO_FS_2000);	// set gyro range to +/- 250 deg/second
	setFullScaleAccelRange(MPU60X0_ACCEL_FS_2);		// set accel range to +- 2g
	//setFilt9250(DLPF_BANDWIDTH_92HZ, 4); 
	
    // enable I2C master mode
    if( !writeRegister(MPU60X0_RA_USER_CTRL,I2C_MST_EN) ){
    	Serial.println("Master Mode Set");
    }

	// set the I2C bus speed to 400 kHz
	if( !writeRegister(MPU60X0_RA_I2C_MST_CTRL,MPU60X0_CLOCK_PLL_XGYRO) ){
		Serial.println("I2C Bus Set");
	}

	// check AK8963 WHO AM I register, expected value is 0x48 (decimal 72)
	readAKRegisters(AK8963_RA_WIA, sizeof(buff), &buff[0]);
	if(  buff[0] != 72 ){
        Serial.print(buff[0]); Serial.print(", ");Serial.println("AK does not match");
	}

    /* get the magnetometer calibration */

    // set AK8963 to Power Down
    if( !writeAKRegister(AK8963_RA_CNTL1, AK8963_MODE_POWERDOWN) ){
        Serial.println("AK Not Powered Down");
    }
    delay(100); // long wait between AK8963 mode changes

    // set AK8963 to FUSE ROM access
    if( !writeAKRegister(AK8963_RA_CNTL1, AK8963_MODE_FUSEROM)){
        Serial.println("FUSE ROM Access Not set");
    }

    delay(100); // long wait between AK8963 mode changes
	
    // read the AK8963 ASA registers and compute magnetometer scale factors
	readAKRegisters(AK8963_RA_ASAX, sizeof(buff), &buff[0]);
	//_magScaleX = ((((float)buff[0]) - 128.0f)/(256.0f) + 1.0f) * 4912.0f / 32760.0f; // micro Tesla
	//_magScaleY = ((((float)buff[1]) - 128.0f)/(256.0f) + 1.0f) * 4912.0f / 32760.0f; // micro Tesla
	//_magScaleZ = ((((float)buff[2]) - 128.0f)/(256.0f) + 1.0f) * 4912.0f / 32760.0f; // micro Tesla 
	_magScaleX = buff[0];
	_magScaleY = buff[1];
	_magScaleZ = buff[2];
	//Serial.print(_magScaleX); Serial.print(", "); Serial.print(_magScaleY); 
	//Serial.print(", "); Serial.println(_magScaleZ);
	
    // set AK8963 to Power Down
    if( !writeAKRegister(AK8963_RA_CNTL1, AK8963_MODE_POWERDOWN) ){
        Serial.println("AK Not Powered Down");
    }
    delay(100); // long wait between AK8963 mode changes  

    // set AK8963 to 16 bit resolution, 100 Hz update rate
    if( !writeAKRegister(AK8963_RA_CNTL1, 0x16) ){
        Serial.println("Res Not Set");
    }
    delay(100); // long wait between AK8963 mode changes

    // select clock source to gyro
    if( !writeRegister(MPU60X0_RA_PWR_MGMT_1, MPU60X0_CLOCK_PLL_XGYRO) ){
        Serial.println("Clock Source Not Set");
    }      

    // instruct the MPU9250 to get 7 bytes of data from the AK8963 at the sample rate
    readAKRegisters(AK8963_RA_HXL,sizeof(data),&data[0]);
	//Serial.println((((int16_t)data[1]) << 8) | data[0]);  
    //Serial.println((((int16_t)data[3]) << 8) | data[2]);
    //Serial.println((((int16_t)data[5]) << 8) | data[4]);

    // successful init, return 0
	//Serial.println("FINISHED");
}


/* sets the DLPF and interrupt settings */
int MPU60X0::setFilt9250(mpu9250_dlpf_bandwidth bandwidth, uint8_t SRD){
    uint8_t data[7];

    switch(bandwidth) {
        case DLPF_BANDWIDTH_184HZ:
            if( !writeRegister(MPU9250_RA_ACCEL_CONFIG2, MPU60X0_DLPF_BW_184) ){ // setting accel bandwidth to 184Hz
                return -1;
            } 
            if( !writeRegister(MPU60X0_RA_GYRO_CONFIG, MPU60X0_DLPF_BW_184) ){ // setting gyro bandwidth to 184Hz
                return -1;
            }
            break;

        case DLPF_BANDWIDTH_92HZ:
            if( !writeRegister(MPU9250_RA_ACCEL_CONFIG2, MPU60X0_DLPF_BW_98) ){ // setting accel bandwidth to 92Hz
                return -1;
            } 
            if( !writeRegister(MPU60X0_RA_GYRO_CONFIG, MPU60X0_DLPF_BW_98) ){ // setting gyro bandwidth to 92Hz
                return -1;
            }
            break; 

        case DLPF_BANDWIDTH_41HZ:
            if( !writeRegister(MPU9250_RA_ACCEL_CONFIG2, MPU60X0_DLPF_BW_42) ){ // setting accel bandwidth to 41Hz
                return -1;
            } 
            if( !writeRegister(MPU60X0_RA_GYRO_CONFIG, MPU60X0_DLPF_BW_42) ){ // setting gyro bandwidth to 41Hz
                return -1;
            } 
            break;

        case DLPF_BANDWIDTH_20HZ:
            if( !writeRegister(MPU9250_RA_ACCEL_CONFIG2, MPU60X0_DLPF_BW_20) ){ // setting accel bandwidth to 20Hz
                return -1;
            } 
            if( !writeRegister(MPU60X0_RA_GYRO_CONFIG, MPU60X0_DLPF_BW_20) ){ // setting gyro bandwidth to 20Hz
                return -1;
            }
            break;

        case DLPF_BANDWIDTH_10HZ:
            if( !writeRegister(MPU9250_RA_ACCEL_CONFIG2,MPU60X0_DLPF_BW_10) ){ // setting accel bandwidth to 10Hz
                return -1;
            } 
            if( !writeRegister(MPU60X0_RA_GYRO_CONFIG,MPU60X0_DLPF_BW_10) ){ // setting gyro bandwidth to 10Hz
                return -1;
            }
            break;

        case DLPF_BANDWIDTH_5HZ:
            if( !writeRegister(MPU9250_RA_ACCEL_CONFIG2,MPU60X0_DLPF_BW_5) ){ // setting accel bandwidth to 5Hz
                return -1;
            } 
            if( !writeRegister(MPU60X0_RA_GYRO_CONFIG,MPU60X0_DLPF_BW_5) ){ // setting gyro bandwidth to 5Hz
                return -1;
            }
            break; 
    }

    /* setting the sample rate divider */
    if( !writeRegister(MPU60X0_RA_SMPLRT_DIV,SRD) ){ // setting the sample rate divider
        return -1;
    } 

    if(SRD > 9){

        // set AK8963 to Power Down
        if( !writeAKRegister(AK8963_RA_CNTL1, AK8963_MODE_POWERDOWN) ){
            return -1;
        }
        delay(100); // long wait between AK8963 mode changes  

        // set AK8963 to 16 bit resolution, 8 Hz update rate
        if( !writeAKRegister(AK8963_RA_CNTL1,0x12) ){
            return -1;
        }
        delay(100); // long wait between AK8963 mode changes     

        // instruct the MPU9250 to get 7 bytes of data from the AK8963 at the sample rate
        readAKRegisters(AK8963_RA_HXL,sizeof(data),&data[0]);
    }

    /* setting the interrupt */
    if( !writeRegister(MPU60X0_RA_INT_PIN_CFG,INT_PULSE_50US) ){ // setup interrupt, 50 us pulse
        return -1;
    }  
    if( !writeRegister(MPU60X0_RA_INT_ENABLE,INT_RAW_RDY_EN) ){ // set to data ready
        return -1;
    }  

    // successful filter setup, return 0
    return 0; 
}

/* enables and disables the interrupt */
int MPU60X0::enableInt9250(bool enable){

	if(enable){
		/* setting the interrupt */
	    if( !writeRegister(MPU60X0_RA_INT_PIN_CFG,INT_PULSE_50US) ){ // setup interrupt, 50 us pulse
	        return -1;
	    }  
	    if( !writeRegister(MPU60X0_RA_INT_ENABLE,INT_RAW_RDY_EN) ){ // set to data ready
	        return -1;
	    }  
	}
	else{
	    if( !writeRegister(MPU60X0_RA_INT_ENABLE,INT_DISABLE) ){ // disable interrupt
	        return -1;
	    }  
	}

    // successful interrupt setup, return 0
    return 0; 
}




/* get accelerometer data given pointers to store the three values, return data as counts */
void MPU60X0::get9250AccelCounts(int16_t* ax, int16_t* ay, int16_t* az){
    uint8_t buff[6];
    int16_t axx, ayy, azz;

    readRegister(MPU60X0_RA_ACCEL_XOUT_H, sizeof(buff), &buff[0]); // grab the data from the MPU9250

    axx = (((int16_t)buff[0]) << 8) | buff[1];  // combine into 16 bit values
    ayy = (((int16_t)buff[2]) << 8) | buff[3];
    azz = (((int16_t)buff[4]) << 8) | buff[5];

    *ax = tX[0]*axx + tX[1]*ayy + tX[2]*azz; // transform axes
    *ay = tY[0]*axx + tY[1]*ayy + tY[2]*azz;
    *az = tZ[0]*axx + tZ[1]*ayy + tZ[2]*azz;
}


/* get gyro data given pointers to store the three values, return data as counts */
void MPU60X0::get9250GyroCounts(int16_t* gx, int16_t* gy, int16_t* gz){
    uint8_t buff[6];
    int16_t gxx, gyy, gzz;

    readRegister(MPU60X0_RA_GYRO_XOUT_H, sizeof(buff), &buff[0]); // grab the data from the MPU9250

    gxx = (((int16_t)buff[0]) << 8) | buff[1];  // combine into 16 bit values
    gyy = (((int16_t)buff[2]) << 8) | buff[3];
    gzz = (((int16_t)buff[4]) << 8) | buff[5];

    *gx = tX[0]*gxx + tX[1]*gyy + tX[2]*gzz; // transform axes
    *gy = tY[0]*gxx + tY[1]*gyy + tY[2]*gzz;
    *gz = tZ[0]*gxx + tZ[1]*gyy + tZ[2]*gzz;
}

/* get magnetometer data given pointers to store the three values, return data as counts */
void MPU60X0::get9250MagCounts(int16_t* hx, int16_t* hy, int16_t* hz){
    uint8_t buff[7];
    // read the magnetometer data off the external sensor buffer
    readRegister(MPU60X0_RA_EXT_SENS_DATA_00,sizeof(buff),&buff[0]);

    if( buff[6] == 0x10 ) { // check for overflow
        *hx = (((int16_t)buff[1]) << 8) | buff[0];  // combine into 16 bit values
        *hy = (((int16_t)buff[3]) << 8) | buff[2];
        *hz = (((int16_t)buff[5]) << 8) | buff[4];
    }
    else{
        *hx = 0;  
        *hy = 0;
        *hz = 0;
    }
}

/* get temperature data given pointer to store the value, return data as counts */
void MPU60X0::get9250TempCounts(int16_t* t){
    uint8_t buff[2];

    readRegister(MPU60X0_RA_TEMP_OUT_H, sizeof(buff), &buff[0]); // grab the data from the MPU9250

    *t = (((int16_t)buff[0]) << 8) | buff[1];  // combine into 16 bit value and return
}

void MPU60X0::get9250Motion9Counts(int16_t* ax, int16_t* ay, int16_t* az, int16_t* gx, int16_t* gy, int16_t* gz, int16_t* hx, int16_t* hy, int16_t* hz)
{
    uint8_t buff[21];
    int16_t axx, ayy, azz, gxx, gyy, gzz;
    readRegister(MPU60X0_RA_ACCEL_XOUT_H, sizeof(buff), &buff[0]); // grab the data from the MPU9250

    axx = (((int16_t)buff[0]) << 8) | buff[1];  // combine into 16 bit values
    ayy = (((int16_t)buff[2]) << 8) | buff[3];
    azz = (((int16_t)buff[4]) << 8) | buff[5];

    gxx = (((int16_t)buff[8]) << 8) | buff[9];
    gyy = (((int16_t)buff[10]) << 8) | buff[11];
    gzz = (((int16_t)buff[12]) << 8) | buff[13];

    *hx = (((int16_t)buff[15]) << 8) | buff[14];  
    *hy = (((int16_t)buff[17]) << 8) | buff[16];
    *hz = (((int16_t)buff[19]) << 8) | buff[18];

    *ax = tX[0]*axx + tX[1]*ayy + tX[2]*azz; // transform axes
    *ay = tY[0]*axx + tY[1]*ayy + tY[2]*azz;
    *az = tZ[0]*axx + tZ[1]*ayy + tZ[2]*azz;

    *gx = tX[0]*gxx + tX[1]*gyy + tX[2]*gzz;
    *gy = tY[0]*gxx + tY[1]*gyy + tY[2]*gzz;
    *gz = tZ[0]*gxx + tZ[1]*gyy + tZ[2]*gzz;
}

/* get accelerometer, gyro, and magnetometer data given pointers to store values */
void MPU60X0::get9250Motion9(float* ax, float* ay, float* az, float* gx, float* gy, float* gz, float* hx, float* hy, float* hz){
    int16_t accel[3];
    int16_t gyro[3];
    int16_t mag[3];

    get9250Motion9Counts(&accel[0], &accel[1], &accel[2], &gyro[0], &gyro[1], &gyro[2], &mag[0], &mag[1], &mag[2]);

    *ax = ((float) accel[0]); // typecast and scale to values
    *ay = ((float) accel[1]);
    *az = ((float) accel[2]);

    *gx = ((float) gyro[0]);
    *gy = ((float) gyro[1]);
    *gz = ((float) gyro[2]);

    *hx = ((float) mag[0]);
    *hy = ((float) mag[1]);
    *hz = ((float) mag[2]);

}

/* get accelerometer, magnetometer, and temperature data given pointers to store values, return data as counts */
void MPU60X0::get9250Motion10Counts(int16_t* ax, int16_t* ay, int16_t* az, int16_t* gx, int16_t* gy, int16_t* gz, int16_t* hx, int16_t* hy, int16_t* hz, int16_t* t){
    uint8_t buff[21];
    int16_t axx, ayy, azz, gxx, gyy, gzz;

    readRegister(MPU60X0_RA_ACCEL_XOUT_H, sizeof(buff), &buff[0]); // grab the data from the MPU9250

    axx = (((int16_t)buff[0]) << 8) | buff[1];  // combine into 16 bit values
    ayy = (((int16_t)buff[2]) << 8) | buff[3];
    azz = (((int16_t)buff[4]) << 8) | buff[5];

    *t = (((int16_t)buff[6]) << 8) | buff[7];

    gxx = (((int16_t)buff[8]) << 8) | buff[9];
    gyy = (((int16_t)buff[10]) << 8) | buff[11];
    gzz = (((int16_t)buff[12]) << 8) | buff[13];

    *hx = (((int16_t)buff[15]) << 8) | buff[14];
    *hy = (((int16_t)buff[17]) << 8) | buff[16];
    *hz = (((int16_t)buff[19]) << 8) | buff[18];

    *ax = tX[0]*axx + tX[1]*ayy + tX[2]*azz; // transform axes
    *ay = tY[0]*axx + tY[1]*ayy + tY[2]*azz;
    *az = tZ[0]*axx + tZ[1]*ayy + tZ[2]*azz;

    *gx = tX[0]*gxx + tX[1]*gyy + tX[2]*gzz;
    *gy = tY[0]*gxx + tY[1]*gyy + tY[2]*gzz;
    *gz = tZ[0]*gxx + tZ[1]*gyy + tZ[2]*gzz;
}


//========================================================
void MPU60X0::readRegister(uint8_t subAddress, uint8_t count, uint8_t* dest){
        Wire.beginTransmission(devAddr); // open the device
        Wire.write(subAddress); // specify the starting register address
        Wire.endTransmission(false);

        Wire.requestFrom(devAddr, count); // specify the number of bytes to receive
		
        //uint8_t i = 0; // read the data into the buffer
        //while( Wire.available() ){
        //    dest[i++] = Wire.read();
        //}
		Wire.readBytes(dest, count);

		
}

/* writes a register to the AK8963 given a register address and data */
bool MPU60X0::writeAKRegister(uint8_t subAddress, uint8_t data){
	uint8_t count = 1;
	uint8_t buff[1];
	
	writeRegister(MPU60X0_RA_I2C_SLV0_ADDR, magDevAddr); // set slave 0 to the AK8963 and set for write
	writeRegister(MPU60X0_RA_I2C_SLV0_REG,subAddress); // set the register to the desired AK8963 sub address
	writeRegister(MPU60X0_RA_I2C_SLV0_DO, data); // store the data for write
	writeRegister(MPU60X0_RA_I2C_SLV0_CTRL, I2C_SLV0_EN | count); // enable I2C and send 1 byte

	// read the register and confirm
	readAKRegisters(subAddress, sizeof(buff), &buff[0]);

	if(buff[0] == data) {
  		return true;
  	}
  	else{
  		return false;
  	}
}


/* reads registers from the AK8963 */
void MPU60X0::readAKRegisters(uint8_t subAddress, uint8_t count, uint8_t* dest){
	
	writeRegister(MPU60X0_RA_I2C_SLV0_ADDR, magDevAddr | I2C_READ_FLAG); // set slave 0 to the AK8963 and set for read
	writeRegister(MPU60X0_RA_I2C_SLV0_REG,  subAddress); // set the register to the desired AK8963 sub address
	writeRegister(MPU60X0_RA_I2C_SLV0_CTRL, I2C_SLV0_EN | count); // enable I2C and request the bytes
	delayMicroseconds(100); // takes some time for these registers to fill
	readRegister(MPU60X0_RA_EXT_SENS_DATA_00, count, dest); // read the bytes off the MPU9250 EXT_SENS_DATA registers

}

bool MPU60X0::writeRegister(uint8_t subAddress, uint8_t data){
	uint8_t buff[1];

    Wire.beginTransmission(devAddr); // open the device
    Wire.write(subAddress); // write the register address
    Wire.write(data); // write the data
    Wire.endTransmission();
   
    delay(10); // need to slow down how fast I write to MPU9250
	
  	/* read back the register */
  	readRegister(subAddress,sizeof(buff),&buff[0]);

  	/* check the read back register against the written register */
  	if(buff[0] == data) {
  		return true;
  	}
  	else{
  		return false;
  	}
	
}


/** Verify the I2C/SPI connection.
 * Make sure the device is connected and responds as expected.
 * @return True if connection is valid, false otherwise
 */
bool MPU60X0::testConnection() {
    return getDeviceID() == 0b110100 || getDeviceID() == 0x71;
}

// AUX_VDDIO register (InvenSense demo code calls this RA_*G_OFFS_TC)

/** Get the auxiliary I2C supply voltage level.
 * When set to 1, the auxiliary I2C bus high logic level is VDD. When cleared to
 * 0, the auxiliary I2C bus high logic level is VLOGIC. This does not apply to
 * the MPU-6000, which does not have a VLOGIC pin.
 * @return I2C supply voltage level (0=VLOGIC, 1=VDD)
 */
uint8_t MPU60X0::getAuxVDDIOLevel() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_YG_OFFS_TC, MPU60X0_TC_PWR_MODE_BIT, buffer);
    return buffer[0];
}
/** Set the auxiliary I2C supply voltage level.
 * When set to 1, the auxiliary I2C bus high logic level is VDD. When cleared to
 * 0, the auxiliary I2C bus high logic level is VLOGIC. This does not apply to
 * the MPU-6000, which does not have a VLOGIC pin.
 * @param level I2C supply voltage level (0=VLOGIC, 1=VDD)
 */
void MPU60X0::setAuxVDDIOLevel(uint8_t level) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_YG_OFFS_TC, MPU60X0_TC_PWR_MODE_BIT, level);
}

// SMPLRT_DIV register

/** Get gyroscope output rate divider.
 * The sensor register output, FIFO output, DMP sampling, Motion detection, Zero
 * Motion detection, and Free Fall detection are all based on the Sample Rate.
 * The Sample Rate is generated by dividing the gyroscope output rate by
 * SMPLRT_DIV:
 *
 * Sample Rate = Gyroscope Output Rate / (1 + SMPLRT_DIV)
 *
 * where Gyroscope Output Rate = 8kHz when the DLPF is disabled (DLPF_CFG = 0 or
 * 7), and 1kHz when the DLPF is enabled (see Register 26).
 *
 * Note: The accelerometer output rate is 1kHz. This means that for a Sample
 * Rate greater than 1kHz, the same accelerometer sample may be output to the
 * FIFO, DMP, and sensor registers more than once.
 *
 * For a diagram of the gyroscope and accelerometer signal paths, see Section 8
 * of the MPU-6000/MPU-6050 Product Specification document.
 *
 * @return Current sample rate
 * @see MPU60X0_RA_SMPLRT_DIV
 */
uint8_t MPU60X0::getRate() {
    return I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_SMPLRT_DIV, buffer);
    return buffer[0];
}
/** Set gyroscope sample rate divider.
 * @param rate New sample rate divider
 * @see getRate()
 * @see MPU60X0_RA_SMPLRT_DIV
 */
void MPU60X0::setRate(uint8_t rate) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_SMPLRT_DIV, rate);
}

// CONFIG register

/** Get external FSYNC configuration.
 * Configures the external Frame Synchronization (FSYNC) pin sampling. An
 * external signal connected to the FSYNC pin can be sampled by configuring
 * EXT_SYNC_SET. Signal changes to the FSYNC pin are latched so that short
 * strobes may be captured. The latched FSYNC signal will be sampled at the
 * Sampling Rate, as defined in register 25. After sampling, the latch will
 * reset to the current FSYNC signal state.
 *
 * The sampled value will be reported in place of the least significant bit in
 * a sensor data register determined by the value of EXT_SYNC_SET according to
 * the following table.
 *
 * <pre>
 * EXT_SYNC_SET | FSYNC Bit Location
 * -------------+-------------------
 * 0            | Input disabled
 * 1            | TEMP_OUT_L[0]
 * 2            | GYRO_XOUT_L[0]
 * 3            | GYRO_YOUT_L[0]
 * 4            | GYRO_ZOUT_L[0]
 * 5            | ACCEL_XOUT_L[0]
 * 6            | ACCEL_YOUT_L[0]
 * 7            | ACCEL_ZOUT_L[0]
 * </pre>
 *
 * @return FSYNC configuration value
 */
uint8_t MPU60X0::getExternalFrameSync() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_CONFIG, MPU60X0_CFG_EXT_SYNC_SET_BIT, MPU60X0_CFG_EXT_SYNC_SET_LENGTH, buffer);
    return buffer[0];
}
/** Set external FSYNC configuration.
 * @see getExternalFrameSync()
 * @see MPU60X0_RA_CONFIG
 * @param sync New FSYNC configuration value
 */
void MPU60X0::setExternalFrameSync(uint8_t sync) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_CONFIG, MPU60X0_CFG_EXT_SYNC_SET_BIT, MPU60X0_CFG_EXT_SYNC_SET_LENGTH, sync);
}
/** Get digital low-pass filter configuration.
 * The DLPF_CFG parameter sets the digital low pass filter configuration. It
 * also determines the internal sampling rate used by the device as shown in
 * the table below.
 *
 * Note: The accelerometer output rate is 1kHz. This means that for a Sample
 * Rate greater than 1kHz, the same accelerometer sample may be output to the
 * FIFO, DMP, and sensor registers more than once.
 *
 * <pre>
 *          |   ACCELEROMETER    |           GYROSCOPE
 * DLPF_CFG | Bandwidth | Delay  | Bandwidth | Delay  | Sample Rate
 * ---------+-----------+--------+-----------+--------+-------------
 * 0        | 260Hz     | 0ms    | 256Hz     | 0.98ms | 8kHz
 * 1        | 184Hz     | 2.0ms  | 188Hz     | 1.9ms  | 1kHz
 * 2        | 94Hz      | 3.0ms  | 98Hz      | 2.8ms  | 1kHz
 * 3        | 44Hz      | 4.9ms  | 42Hz      | 4.8ms  | 1kHz
 * 4        | 21Hz      | 8.5ms  | 20Hz      | 8.3ms  | 1kHz
 * 5        | 10Hz      | 13.8ms | 10Hz      | 13.4ms | 1kHz
 * 6        | 5Hz       | 19.0ms | 5Hz       | 18.6ms | 1kHz
 * 7        |   -- Reserved --   |   -- Reserved --   | Reserved
 * </pre>
 *
 * @return DLFP configuration
 * @see MPU60X0_RA_CONFIG
 * @see MPU60X0_CFG_DLPF_CFG_BIT
 * @see MPU60X0_CFG_DLPF_CFG_LENGTH
 */
uint8_t MPU60X0::getDLPFMode() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_CONFIG, MPU60X0_CFG_DLPF_CFG_BIT, MPU60X0_CFG_DLPF_CFG_LENGTH, buffer);
    return buffer[0];
}
/** Set digital low-pass filter configuration.
 * @param mode New DLFP configuration setting
 * @see getDLPFBandwidth()
 * @see MPU60X0_DLPF_BW_256
 * @see MPU60X0_RA_CONFIG
 * @see MPU60X0_CFG_DLPF_CFG_BIT
 * @see MPU60X0_CFG_DLPF_CFG_LENGTH
 */
void MPU60X0::setDLPFMode(uint8_t mode) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_CONFIG, MPU60X0_CFG_DLPF_CFG_BIT, MPU60X0_CFG_DLPF_CFG_LENGTH, mode);
}

// GYRO_CONFIG register

/** Get full-scale gyroscope range.
 * The FS_SEL parameter allows setting the full-scale range of the gyro sensors,
 * as described in the table below.
 *
 * <pre>
 * 0 = +/- 250 degrees/sec
 * 1 = +/- 500 degrees/sec
 * 2 = +/- 1000 degrees/sec
 * 3 = +/- 2000 degrees/sec
 * </pre>
 *
 * @return Current full-scale gyroscope range setting
 * @see MPU60X0_GYRO_FS_250
 * @see MPU60X0_RA_GYRO_CONFIG
 * @see MPU60X0_GCONFIG_FS_SEL_BIT
 * @see MPU60X0_GCONFIG_FS_SEL_LENGTH
 */
uint8_t MPU60X0::getFullScaleGyroRange() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_GYRO_CONFIG, MPU60X0_GCONFIG_FS_SEL_BIT, MPU60X0_GCONFIG_FS_SEL_LENGTH, buffer);
    return buffer[0];
}
/** Set full-scale gyroscope range.
 * @param range New full-scale gyroscope range value
 * @see getFullScaleRange()
 * @see MPU60X0_GYRO_FS_250
 * @see MPU60X0_RA_GYRO_CONFIG
 * @see MPU60X0_GCONFIG_FS_SEL_BIT
 * @see MPU60X0_GCONFIG_FS_SEL_LENGTH
 */
void MPU60X0::setFullScaleGyroRange(uint8_t range) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_GYRO_CONFIG, MPU60X0_GCONFIG_FS_SEL_BIT, MPU60X0_GCONFIG_FS_SEL_LENGTH, range);
}

// ACCEL_CONFIG register

/** Get self-test enabled setting for accelerometer X axis.
 * @return Self-test enabled value
 * @see MPU60X0_RA_ACCEL_CONFIG
 */
bool MPU60X0::getAccelXSelfTest() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_ACCEL_CONFIG, MPU60X0_ACONFIG_XA_ST_BIT, buffer);
    return buffer[0];
}
/** Get self-test enabled setting for accelerometer X axis.
 * @param enabled Self-test enabled value
 * @see MPU60X0_RA_ACCEL_CONFIG
 */
void MPU60X0::setAccelXSelfTest(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_ACCEL_CONFIG, MPU60X0_ACONFIG_XA_ST_BIT, enabled);
}
/** Get self-test enabled value for accelerometer Y axis.
 * @return Self-test enabled value
 * @see MPU60X0_RA_ACCEL_CONFIG
 */
bool MPU60X0::getAccelYSelfTest() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_ACCEL_CONFIG, MPU60X0_ACONFIG_YA_ST_BIT, buffer);
    return buffer[0];
}
/** Get self-test enabled value for accelerometer Y axis.
 * @param enabled Self-test enabled value
 * @see MPU60X0_RA_ACCEL_CONFIG
 */
void MPU60X0::setAccelYSelfTest(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_ACCEL_CONFIG, MPU60X0_ACONFIG_YA_ST_BIT, enabled);
}
/** Get self-test enabled value for accelerometer Z axis.
 * @return Self-test enabled value
 * @see MPU60X0_RA_ACCEL_CONFIG
 */
bool MPU60X0::getAccelZSelfTest() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_ACCEL_CONFIG, MPU60X0_ACONFIG_ZA_ST_BIT, buffer);
    return buffer[0];
}
/** Set self-test enabled value for accelerometer Z axis.
 * @param enabled Self-test enabled value
 * @see MPU60X0_RA_ACCEL_CONFIG
 */
void MPU60X0::setAccelZSelfTest(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_ACCEL_CONFIG, MPU60X0_ACONFIG_ZA_ST_BIT, enabled);
}
/** Get full-scale accelerometer range.
 * The FS_SEL parameter allows setting the full-scale range of the accelerometer
 * sensors, as described in the table below.
 *
 * <pre>
 * 0 = +/- 2g
 * 1 = +/- 4g
 * 2 = +/- 8g
 * 3 = +/- 16g
 * </pre>
 *
 * @return Current full-scale accelerometer range setting
 * @see MPU60X0_ACCEL_FS_2
 * @see MPU60X0_RA_ACCEL_CONFIG
 * @see MPU60X0_ACONFIG_AFS_SEL_BIT
 * @see MPU60X0_ACONFIG_AFS_SEL_LENGTH
 */
uint8_t MPU60X0::getFullScaleAccelRange() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_ACCEL_CONFIG, MPU60X0_ACONFIG_AFS_SEL_BIT, MPU60X0_ACONFIG_AFS_SEL_LENGTH, buffer);
    return buffer[0];
}
/** Set full-scale accelerometer range.
 * @param range New full-scale accelerometer range setting
 * @see getFullScaleAccelRange()
 */
void MPU60X0::setFullScaleAccelRange(uint8_t range) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_ACCEL_CONFIG, MPU60X0_ACONFIG_AFS_SEL_BIT, MPU60X0_ACONFIG_AFS_SEL_LENGTH, range);
}
/** Get the high-pass filter configuration.
 * The DHPF is a filter module in the path leading to motion detectors (Free
 * Fall, Motion threshold, and Zero Motion). The high pass filter output is not
 * available to the data registers (see Figure in Section 8 of the MPU-6000/
 * MPU-6050 Product Specification document).
 *
 * The high pass filter has three modes:
 *
 * <pre>
 *    Reset: The filter output settles to zero within one sample. This
 *           effectively disables the high pass filter. This mode may be toggled
 *           to quickly settle the filter.
 *
 *    On:    The high pass filter will pass signals above the cut off frequency.
 *
 *    Hold:  When triggered, the filter holds the present sample. The filter
 *           output will be the difference between the input sample and the held
 *           sample.
 * </pre>
 *
 * <pre>
 * ACCEL_HPF | Filter Mode | Cut-off Frequency
 * ----------+-------------+------------------
 * 0         | Reset       | None
 * 1         | On          | 5Hz
 * 2         | On          | 2.5Hz
 * 3         | On          | 1.25Hz
 * 4         | On          | 0.63Hz
 * 7         | Hold        | None
 * </pre>
 *
 * @return Current high-pass filter configuration
 * @see MPU60X0_DHPF_RESET
 * @see MPU60X0_RA_ACCEL_CONFIG
 */
uint8_t MPU60X0::getDHPFMode() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_ACCEL_CONFIG, MPU60X0_ACONFIG_ACCEL_HPF_BIT, MPU60X0_ACONFIG_ACCEL_HPF_LENGTH, buffer);
    return buffer[0];
}
/** Set the high-pass filter configuration.
 * @param bandwidth New high-pass filter configuration
 * @see setDHPFMode()
 * @see MPU60X0_DHPF_RESET
 * @see MPU60X0_RA_ACCEL_CONFIG
 */
void MPU60X0::setDHPFMode(uint8_t bandwidth) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_ACCEL_CONFIG, MPU60X0_ACONFIG_ACCEL_HPF_BIT, MPU60X0_ACONFIG_ACCEL_HPF_LENGTH, bandwidth);
}

// FF_THR register

/** Get free-fall event acceleration threshold.
 * This register configures the detection threshold for Free Fall event
 * detection. The unit of FF_THR is 1LSB = 2mg. Free Fall is detected when the
 * absolute value of the accelerometer measurements for the three axes are each
 * less than the detection threshold. This condition increments the Free Fall
 * duration counter (Register 30). The Free Fall interrupt is triggered when the
 * Free Fall duration counter reaches the time specified in FF_DUR.
 *
 * For more details on the Free Fall detection interrupt, see Section 8.2 of the
 * MPU-6000/MPU-6050 Product Specification document as well as Registers 56 and
 * 58 of this document.
 *
 * @return Current free-fall acceleration threshold value (LSB = 2mg)
 * @see MPU60X0_RA_FF_THR
 */
uint8_t MPU60X0::getFreefallDetectionThreshold() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_FF_THR, buffer);
    return buffer[0];
}
/** Get free-fall event acceleration threshold.
 * @param threshold New free-fall acceleration threshold value (LSB = 2mg)
 * @see getFreefallDetectionThreshold()
 * @see MPU60X0_RA_FF_THR
 */
void MPU60X0::setFreefallDetectionThreshold(uint8_t threshold) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_FF_THR, threshold);
}

// FF_DUR register

/** Get free-fall event duration threshold.
 * This register configures the duration counter threshold for Free Fall event
 * detection. The duration counter ticks at 1kHz, therefore FF_DUR has a unit
 * of 1 LSB = 1 ms.
 *
 * The Free Fall duration counter increments while the absolute value of the
 * accelerometer measurements are each less than the detection threshold
 * (Register 29). The Free Fall interrupt is triggered when the Free Fall
 * duration counter reaches the time specified in this register.
 *
 * For more details on the Free Fall detection interrupt, see Section 8.2 of
 * the MPU-6000/MPU-6050 Product Specification document as well as Registers 56
 * and 58 of this document.
 *
 * @return Current free-fall duration threshold value (LSB = 1ms)
 * @see MPU60X0_RA_FF_DUR
 */
uint8_t MPU60X0::getFreefallDetectionDuration() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_FF_DUR, buffer);
    return buffer[0];
}
/** Get free-fall event duration threshold.
 * @param duration New free-fall duration threshold value (LSB = 1ms)
 * @see getFreefallDetectionDuration()
 * @see MPU60X0_RA_FF_DUR
 */
void MPU60X0::setFreefallDetectionDuration(uint8_t duration) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_FF_DUR, duration);
}

// MOT_THR register

/** Get motion detection event acceleration threshold.
 * This register configures the detection threshold for Motion interrupt
 * generation. The unit of MOT_THR is 1LSB = 2mg. Motion is detected when the
 * absolute value of any of the accelerometer measurements exceeds this Motion
 * detection threshold. This condition increments the Motion detection duration
 * counter (Register 32). The Motion detection interrupt is triggered when the
 * Motion Detection counter reaches the time count specified in MOT_DUR
 * (Register 32).
 *
 * The Motion interrupt will indicate the axis and polarity of detected motion
 * in MOT_DETECT_STATUS (Register 97).
 *
 * For more details on the Motion detection interrupt, see Section 8.3 of the
 * MPU-6000/MPU-6050 Product Specification document as well as Registers 56 and
 * 58 of this document.
 *
 * @return Current motion detection acceleration threshold value (LSB = 2mg)
 * @see MPU60X0_RA_MOT_THR
 */
uint8_t MPU60X0::getMotionDetectionThreshold() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_MOT_THR, buffer);
    return buffer[0];
}
/** Set free-fall event acceleration threshold.
 * @param threshold New motion detection acceleration threshold value (LSB = 2mg)
 * @see getMotionDetectionThreshold()
 * @see MPU60X0_RA_MOT_THR
 */
void MPU60X0::setMotionDetectionThreshold(uint8_t threshold) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_MOT_THR, threshold);
}

// MOT_DUR register

/** Get motion detection event duration threshold.
 * This register configures the duration counter threshold for Motion interrupt
 * generation. The duration counter ticks at 1 kHz, therefore MOT_DUR has a unit
 * of 1LSB = 1ms. The Motion detection duration counter increments when the
 * absolute value of any of the accelerometer measurements exceeds the Motion
 * detection threshold (Register 31). The Motion detection interrupt is
 * triggered when the Motion detection counter reaches the time count specified
 * in this register.
 *
 * For more details on the Motion detection interrupt, see Section 8.3 of the
 * MPU-6000/MPU-6050 Product Specification document.
 *
 * @return Current motion detection duration threshold value (LSB = 1ms)
 * @see MPU60X0_RA_MOT_DUR
 */
uint8_t MPU60X0::getMotionDetectionDuration() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_MOT_DUR, buffer);
    return buffer[0];
}
/** Set motion detection event duration threshold.
 * @param duration New motion detection duration threshold value (LSB = 1ms)
 * @see getMotionDetectionDuration()
 * @see MPU60X0_RA_MOT_DUR
 */
void MPU60X0::setMotionDetectionDuration(uint8_t duration) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_MOT_DUR, duration);
}

// ZRMOT_THR register

/** Get zero motion detection event acceleration threshold.
 * This register configures the detection threshold for Zero Motion interrupt
 * generation. The unit of ZRMOT_THR is 1LSB = 2mg. Zero Motion is detected when
 * the absolute value of the accelerometer measurements for the 3 axes are each
 * less than the detection threshold. This condition increments the Zero Motion
 * duration counter (Register 34). The Zero Motion interrupt is triggered when
 * the Zero Motion duration counter reaches the time count specified in
 * ZRMOT_DUR (Register 34).
 *
 * Unlike Free Fall or Motion detection, Zero Motion detection triggers an
 * interrupt both when Zero Motion is first detected and when Zero Motion is no
 * longer detected.
 *
 * When a zero motion event is detected, a Zero Motion Status will be indicated
 * in the MOT_DETECT_STATUS register (Register 97). When a motion-to-zero-motion
 * condition is detected, the status bit is set to 1. When a zero-motion-to-
 * motion condition is detected, the status bit is set to 0.
 *
 * For more details on the Zero Motion detection interrupt, see Section 8.4 of
 * the MPU-6000/MPU-6050 Product Specification document as well as Registers 56
 * and 58 of this document.
 *
 * @return Current zero motion detection acceleration threshold value (LSB = 2mg)
 * @see MPU60X0_RA_ZRMOT_THR
 */
uint8_t MPU60X0::getZeroMotionDetectionThreshold() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_ZRMOT_THR, buffer);
    return buffer[0];
}
/** Set zero motion detection event acceleration threshold.
 * @param threshold New zero motion detection acceleration threshold value (LSB = 2mg)
 * @see getZeroMotionDetectionThreshold()
 * @see MPU60X0_RA_ZRMOT_THR
 */
void MPU60X0::setZeroMotionDetectionThreshold(uint8_t threshold) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_ZRMOT_THR, threshold);
}

// ZRMOT_DUR register

/** Get zero motion detection event duration threshold.
 * This register configures the duration counter threshold for Zero Motion
 * interrupt generation. The duration counter ticks at 16 Hz, therefore
 * ZRMOT_DUR has a unit of 1 LSB = 64 ms. The Zero Motion duration counter
 * increments while the absolute value of the accelerometer measurements are
 * each less than the detection threshold (Register 33). The Zero Motion
 * interrupt is triggered when the Zero Motion duration counter reaches the time
 * count specified in this register.
 *
 * For more details on the Zero Motion detection interrupt, see Section 8.4 of
 * the MPU-6000/MPU-6050 Product Specification document, as well as Registers 56
 * and 58 of this document.
 *
 * @return Current zero motion detection duration threshold value (LSB = 64ms)
 * @see MPU60X0_RA_ZRMOT_DUR
 */
uint8_t MPU60X0::getZeroMotionDetectionDuration() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_ZRMOT_DUR, buffer);
    return buffer[0];
}
/** Set zero motion detection event duration threshold.
 * @param duration New zero motion detection duration threshold value (LSB = 1ms)
 * @see getZeroMotionDetectionDuration()
 * @see MPU60X0_RA_ZRMOT_DUR
 */
void MPU60X0::setZeroMotionDetectionDuration(uint8_t duration) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_ZRMOT_DUR, duration);
}

// FIFO_EN register

/** Get temperature FIFO enabled value.
 * When set to 1, this bit enables TEMP_OUT_H and TEMP_OUT_L (Registers 65 and
 * 66) to be written into the FIFO buffer.
 * @return Current temperature FIFO enabled value
 * @see MPU60X0_RA_FIFO_EN
 */
bool MPU60X0::getTempFIFOEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_TEMP_FIFO_EN_BIT, buffer);
    return buffer[0];
}
/** Set temperature FIFO enabled value.
 * @param enabled New temperature FIFO enabled value
 * @see getTempFIFOEnabled()
 * @see MPU60X0_RA_FIFO_EN
 */
void MPU60X0::setTempFIFOEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_TEMP_FIFO_EN_BIT, enabled);
}
/** Get gyroscope X-axis FIFO enabled value.
 * When set to 1, this bit enables GYRO_XOUT_H and GYRO_XOUT_L (Registers 67 and
 * 68) to be written into the FIFO buffer.
 * @return Current gyroscope X-axis FIFO enabled value
 * @see MPU60X0_RA_FIFO_EN
 */
bool MPU60X0::getXGyroFIFOEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_XG_FIFO_EN_BIT, buffer);
    return buffer[0];
}
/** Set gyroscope X-axis FIFO enabled value.
 * @param enabled New gyroscope X-axis FIFO enabled value
 * @see getXGyroFIFOEnabled()
 * @see MPU60X0_RA_FIFO_EN
 */
void MPU60X0::setXGyroFIFOEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_XG_FIFO_EN_BIT, enabled);
}
/** Get gyroscope Y-axis FIFO enabled value.
 * When set to 1, this bit enables GYRO_YOUT_H and GYRO_YOUT_L (Registers 69 and
 * 70) to be written into the FIFO buffer.
 * @return Current gyroscope Y-axis FIFO enabled value
 * @see MPU60X0_RA_FIFO_EN
 */
bool MPU60X0::getYGyroFIFOEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_YG_FIFO_EN_BIT, buffer);
    return buffer[0];
}
/** Set gyroscope Y-axis FIFO enabled value.
 * @param enabled New gyroscope Y-axis FIFO enabled value
 * @see getYGyroFIFOEnabled()
 * @see MPU60X0_RA_FIFO_EN
 */
void MPU60X0::setYGyroFIFOEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_YG_FIFO_EN_BIT, enabled);
}
/** Get gyroscope Z-axis FIFO enabled value.
 * When set to 1, this bit enables GYRO_ZOUT_H and GYRO_ZOUT_L (Registers 71 and
 * 72) to be written into the FIFO buffer.
 * @return Current gyroscope Z-axis FIFO enabled value
 * @see MPU60X0_RA_FIFO_EN
 */
bool MPU60X0::getZGyroFIFOEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_ZG_FIFO_EN_BIT, buffer);
    return buffer[0];
}
/** Set gyroscope Z-axis FIFO enabled value.
 * @param enabled New gyroscope Z-axis FIFO enabled value
 * @see getZGyroFIFOEnabled()
 * @see MPU60X0_RA_FIFO_EN
 */
void MPU60X0::setZGyroFIFOEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_ZG_FIFO_EN_BIT, enabled);
}
/** Get accelerometer FIFO enabled value.
 * When set to 1, this bit enables ACCEL_XOUT_H, ACCEL_XOUT_L, ACCEL_YOUT_H,
 * ACCEL_YOUT_L, ACCEL_ZOUT_H, and ACCEL_ZOUT_L (Registers 59 to 64) to be
 * written into the FIFO buffer.
 * @return Current accelerometer FIFO enabled value
 * @see MPU60X0_RA_FIFO_EN
 */
bool MPU60X0::getAccelFIFOEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_ACCEL_FIFO_EN_BIT, buffer);
    return buffer[0];
}
/** Set accelerometer FIFO enabled value.
 * @param enabled New accelerometer FIFO enabled value
 * @see getAccelFIFOEnabled()
 * @see MPU60X0_RA_FIFO_EN
 */
void MPU60X0::setAccelFIFOEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_ACCEL_FIFO_EN_BIT, enabled);
}
/** Get Slave 2 FIFO enabled value.
 * When set to 1, this bit enables EXT_SENS_DATA registers (Registers 73 to 96)
 * associated with Slave 2 to be written into the FIFO buffer.
 * @return Current Slave 2 FIFO enabled value
 * @see MPU60X0_RA_FIFO_EN
 */
bool MPU60X0::getSlave2FIFOEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_SLV2_FIFO_EN_BIT, buffer);
    return buffer[0];
}
/** Set Slave 2 FIFO enabled value.
 * @param enabled New Slave 2 FIFO enabled value
 * @see getSlave2FIFOEnabled()
 * @see MPU60X0_RA_FIFO_EN
 */
void MPU60X0::setSlave2FIFOEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_SLV2_FIFO_EN_BIT, enabled);
}
/** Get Slave 1 FIFO enabled value.
 * When set to 1, this bit enables EXT_SENS_DATA registers (Registers 73 to 96)
 * associated with Slave 1 to be written into the FIFO buffer.
 * @return Current Slave 1 FIFO enabled value
 * @see MPU60X0_RA_FIFO_EN
 */
bool MPU60X0::getSlave1FIFOEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_SLV1_FIFO_EN_BIT, buffer);
    return buffer[0];
}
/** Set Slave 1 FIFO enabled value.
 * @param enabled New Slave 1 FIFO enabled value
 * @see getSlave1FIFOEnabled()
 * @see MPU60X0_RA_FIFO_EN
 */
void MPU60X0::setSlave1FIFOEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_SLV1_FIFO_EN_BIT, enabled);
}
/** Get Slave 0 FIFO enabled value.
 * When set to 1, this bit enables EXT_SENS_DATA registers (Registers 73 to 96)
 * associated with Slave 0 to be written into the FIFO buffer.
 * @return Current Slave 0 FIFO enabled value
 * @see MPU60X0_RA_FIFO_EN
 */
bool MPU60X0::getSlave0FIFOEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_SLV0_FIFO_EN_BIT, buffer);
    return buffer[0];
}
/** Set Slave 0 FIFO enabled value.
 * @param enabled New Slave 0 FIFO enabled value
 * @see getSlave0FIFOEnabled()
 * @see MPU60X0_RA_FIFO_EN
 */
void MPU60X0::setSlave0FIFOEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_FIFO_EN, MPU60X0_SLV0_FIFO_EN_BIT, enabled);
}

// I2C_MST_CTRL register

/** Get multi-master enabled value.
 * Multi-master capability allows multiple I2C masters to operate on the same
 * bus. In circuits where multi-master capability is required, set MULT_MST_EN
 * to 1. This will increase current drawn by approximately 30uA.
 *
 * In circuits where multi-master capability is required, the state of the I2C
 * bus must always be monitored by each separate I2C Master. Before an I2C
 * Master can assume arbitration of the bus, it must first confirm that no other
 * I2C Master has arbitration of the bus. When MULT_MST_EN is set to 1, the
 * MPU-60X0's bus arbitration detection logic is turned on, enabling it to
 * detect when the bus is available.
 *
 * @return Current multi-master enabled value
 * @see MPU60X0_RA_I2C_MST_CTRL
 */
bool MPU60X0::getMultiMasterEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_CTRL, MPU60X0_MULT_MST_EN_BIT, buffer);
    return buffer[0];
}
/** Set multi-master enabled value.
 * @param enabled New multi-master enabled value
 * @see getMultiMasterEnabled()
 * @see MPU60X0_RA_I2C_MST_CTRL
 */
void MPU60X0::setMultiMasterEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_CTRL, MPU60X0_MULT_MST_EN_BIT, enabled);
}
/** Get wait-for-external-sensor-data enabled value.
 * When the WAIT_FOR_ES bit is set to 1, the Data Ready interrupt will be
 * delayed until External Sensor data from the Slave Devices are loaded into the
 * EXT_SENS_DATA registers. This is used to ensure that both the internal sensor
 * data (i.e. from gyro and accel) and external sensor data have been loaded to
 * their respective data registers (i.e. the data is synced) when the Data Ready
 * interrupt is triggered.
 *
 * @return Current wait-for-external-sensor-data enabled value
 * @see MPU60X0_RA_I2C_MST_CTRL
 */
bool MPU60X0::getWaitForExternalSensorEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_CTRL, MPU60X0_WAIT_FOR_ES_BIT, buffer);
    return buffer[0];
}
/** Set wait-for-external-sensor-data enabled value.
 * @param enabled New wait-for-external-sensor-data enabled value
 * @see getWaitForExternalSensorEnabled()
 * @see MPU60X0_RA_I2C_MST_CTRL
 */
void MPU60X0::setWaitForExternalSensorEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_CTRL, MPU60X0_WAIT_FOR_ES_BIT, enabled);
}
/** Get Slave 3 FIFO enabled value.
 * When set to 1, this bit enables EXT_SENS_DATA registers (Registers 73 to 96)
 * associated with Slave 3 to be written into the FIFO buffer.
 * @return Current Slave 3 FIFO enabled value
 * @see MPU60X0_RA_MST_CTRL
 */
bool MPU60X0::getSlave3FIFOEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_CTRL, MPU60X0_SLV_3_FIFO_EN_BIT, buffer);
    return buffer[0];
}
/** Set Slave 3 FIFO enabled value.
 * @param enabled New Slave 3 FIFO enabled value
 * @see getSlave3FIFOEnabled()
 * @see MPU60X0_RA_MST_CTRL
 */
void MPU60X0::setSlave3FIFOEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_CTRL, MPU60X0_SLV_3_FIFO_EN_BIT, enabled);
}
/** Get slave read/write transition enabled value.
 * The I2C_MST_P_NSR bit configures the I2C Master's transition from one slave
 * read to the next slave read. If the bit equals 0, there will be a restart
 * between reads. If the bit equals 1, there will be a stop followed by a start
 * of the following read. When a write transaction follows a read transaction,
 * the stop followed by a start of the successive write will be always used.
 *
 * @return Current slave read/write transition enabled value
 * @see MPU60X0_RA_I2C_MST_CTRL
 */
bool MPU60X0::getSlaveReadWriteTransitionEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_CTRL, MPU60X0_I2C_MST_P_NSR_BIT, buffer);
    return buffer[0];
}
/** Set slave read/write transition enabled value.
 * @param enabled New slave read/write transition enabled value
 * @see getSlaveReadWriteTransitionEnabled()
 * @see MPU60X0_RA_I2C_MST_CTRL
 */
void MPU60X0::setSlaveReadWriteTransitionEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_CTRL, MPU60X0_I2C_MST_P_NSR_BIT, enabled);
}
/** Get I2C master clock speed.
 * I2C_MST_CLK is a 4 bit unsigned value which configures a divider on the
 * MPU-60X0 internal 8MHz clock. It sets the I2C master clock speed according to
 * the following table:
 *
 * <pre>
 * I2C_MST_CLK | I2C Master Clock Speed | 8MHz Clock Divider
 * ------------+------------------------+-------------------
 * 0           | 348kHz                 | 23
 * 1           | 333kHz                 | 24
 * 2           | 320kHz                 | 25
 * 3           | 308kHz                 | 26
 * 4           | 296kHz                 | 27
 * 5           | 286kHz                 | 28
 * 6           | 276kHz                 | 29
 * 7           | 267kHz                 | 30
 * 8           | 258kHz                 | 31
 * 9           | 500kHz                 | 16
 * 10          | 471kHz                 | 17
 * 11          | 444kHz                 | 18
 * 12          | 421kHz                 | 19
 * 13          | 400kHz                 | 20
 * 14          | 381kHz                 | 21
 * 15          | 364kHz                 | 22
 * </pre>
 *
 * @return Current I2C master clock speed
 * @see MPU60X0_RA_I2C_MST_CTRL
 */
uint8_t MPU60X0::getMasterClockSpeed() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_I2C_MST_CTRL, MPU60X0_I2C_MST_CLK_BIT, MPU60X0_I2C_MST_CLK_LENGTH, buffer);
    return buffer[0];
}
/** Set I2C master clock speed.
 * @reparam speed Current I2C master clock speed
 * @see MPU60X0_RA_I2C_MST_CTRL
 */
void MPU60X0::setMasterClockSpeed(uint8_t speed) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_I2C_MST_CTRL, MPU60X0_I2C_MST_CLK_BIT, MPU60X0_I2C_MST_CLK_LENGTH, speed);
}

// I2C_SLV* registers (Slave 0-3)

/** Get the I2C address of the specified slave (0-3).
 * Note that Bit 7 (MSB) controls read/write mode. If Bit 7 is set, it's a read
 * operation, and if it is cleared, then it's a write operation. The remaining
 * bits (6-0) are the 7-bit device address of the slave device.
 *
 * In read mode, the result of the read is placed in the lowest available 
 * EXT_SENS_DATA register. For further information regarding the allocation of
 * read results, please refer to the EXT_SENS_DATA register description
 * (Registers 73 � 96).
 *
 * The MPU-6050 supports a total of five slaves, but Slave 4 has unique
 * characteristics, and so it has its own functions (getSlave4* and setSlave4*).
 *
 * I2C data transactions are performed at the Sample Rate, as defined in
 * Register 25. The user is responsible for ensuring that I2C data transactions
 * to and from each enabled Slave can be completed within a single period of the
 * Sample Rate.
 *
 * The I2C slave access rate can be reduced relative to the Sample Rate. This
 * reduced access rate is determined by I2C_MST_DLY (Register 52). Whether a
 * slave's access rate is reduced relative to the Sample Rate is determined by
 * I2C_MST_DELAY_CTRL (Register 103).
 *
 * The processing order for the slaves is fixed. The sequence followed for
 * processing the slaves is Slave 0, Slave 1, Slave 2, Slave 3 and Slave 4. If a
 * particular Slave is disabled it will be skipped.
 *
 * Each slave can either be accessed at the sample rate or at a reduced sample
 * rate. In a case where some slaves are accessed at the Sample Rate and some
 * slaves are accessed at the reduced rate, the sequence of accessing the slaves
 * (Slave 0 to Slave 4) is still followed. However, the reduced rate slaves will
 * be skipped if their access rate dictates that they should not be accessed
 * during that particular cycle. For further information regarding the reduced
 * access rate, please refer to Register 52. Whether a slave is accessed at the
 * Sample Rate or at the reduced rate is determined by the Delay Enable bits in
 * Register 103.
 *
 * @param num Slave number (0-3)
 * @return Current address for specified slave
 * @see MPU60X0_RA_I2C_SLV0_ADDR
 */
uint8_t MPU60X0::getSlaveAddress(uint8_t num) {
    if (num > 3) return 0;
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_ADDR + num*3, buffer);
    return buffer[0];
}
/** Set the I2C address of the specified slave (0-3).
 * @param num Slave number (0-3)
 * @param address New address for specified slave
 * @see getSlaveAddress()
 * @see MPU60X0_RA_I2C_SLV0_ADDR
 */
void MPU60X0::setSlaveAddress(uint8_t num, uint8_t address) {
    if (num > 3) return;
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_ADDR + num*3, address);
}
/** Get the active internal register for the specified slave (0-3).
 * Read/write operations for this slave will be done to whatever internal
 * register address is stored in this MPU register.
 *
 * The MPU-6050 supports a total of five slaves, but Slave 4 has unique
 * characteristics, and so it has its own functions.
 *
 * @param num Slave number (0-3)
 * @return Current active register for specified slave
 * @see MPU60X0_RA_I2C_SLV0_REG
 */
uint8_t MPU60X0::getSlaveRegister(uint8_t num) {
    if (num > 3) return 0;
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_REG + num*3, buffer);
    return buffer[0];
}
/** Set the active internal register for the specified slave (0-3).
 * @param num Slave number (0-3)
 * @param reg New active register for specified slave
 * @see getSlaveRegister()
 * @see MPU60X0_RA_I2C_SLV0_REG
 */
void MPU60X0::setSlaveRegister(uint8_t num, uint8_t reg) {
    if (num > 3) return;
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_REG + num*3, reg);
}
/** Get the enabled value for the specified slave (0-3).
 * When set to 1, this bit enables Slave 0 for data transfer operations. When
 * cleared to 0, this bit disables Slave 0 from data transfer operations.
 * @param num Slave number (0-3)
 * @return Current enabled value for specified slave
 * @see MPU60X0_RA_I2C_SLV0_CTRL
 */
bool MPU60X0::getSlaveEnabled(uint8_t num) {
    if (num > 3) return 0;
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_CTRL + num*3, MPU60X0_I2C_SLV_EN_BIT, buffer);
    return buffer[0];
}
/** Set the enabled value for the specified slave (0-3).
 * @param num Slave number (0-3)
 * @param enabled New enabled value for specified slave
 * @see getSlaveEnabled()
 * @see MPU60X0_RA_I2C_SLV0_CTRL
 */
void MPU60X0::setSlaveEnabled(uint8_t num, bool enabled) {
    if (num > 3) return;
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_CTRL + num*3, MPU60X0_I2C_SLV_EN_BIT, enabled);
}
/** Get word pair byte-swapping enabled for the specified slave (0-3).
 * When set to 1, this bit enables byte swapping. When byte swapping is enabled,
 * the high and low bytes of a word pair are swapped. Please refer to
 * I2C_SLV0_GRP for the pairing convention of the word pairs. When cleared to 0,
 * bytes transferred to and from Slave 0 will be written to EXT_SENS_DATA
 * registers in the order they were transferred.
 *
 * @param num Slave number (0-3)
 * @return Current word pair byte-swapping enabled value for specified slave
 * @see MPU60X0_RA_I2C_SLV0_CTRL
 */
bool MPU60X0::getSlaveWordByteSwap(uint8_t num) {
    if (num > 3) return 0;
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_CTRL + num*3, MPU60X0_I2C_SLV_BYTE_SW_BIT, buffer);
    return buffer[0];
}
/** Set word pair byte-swapping enabled for the specified slave (0-3).
 * @param num Slave number (0-3)
 * @param enabled New word pair byte-swapping enabled value for specified slave
 * @see getSlaveWordByteSwap()
 * @see MPU60X0_RA_I2C_SLV0_CTRL
 */
void MPU60X0::setSlaveWordByteSwap(uint8_t num, bool enabled) {
    if (num > 3) return;
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_CTRL + num*3, MPU60X0_I2C_SLV_BYTE_SW_BIT, enabled);
}
/** Get write mode for the specified slave (0-3).
 * When set to 1, the transaction will read or write data only. When cleared to
 * 0, the transaction will write a register address prior to reading or writing
 * data. This should equal 0 when specifying the register address within the
 * Slave device to/from which the ensuing data transaction will take place.
 *
 * @param num Slave number (0-3)
 * @return Current write mode for specified slave (0 = register address + data, 1 = data only)
 * @see MPU60X0_RA_I2C_SLV0_CTRL
 */
bool MPU60X0::getSlaveWriteMode(uint8_t num) {
    if (num > 3) return 0;
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_CTRL + num*3, MPU60X0_I2C_SLV_REG_DIS_BIT, buffer);
    return buffer[0];
}
/** Set write mode for the specified slave (0-3).
 * @param num Slave number (0-3)
 * @param mode New write mode for specified slave (0 = register address + data, 1 = data only)
 * @see getSlaveWriteMode()
 * @see MPU60X0_RA_I2C_SLV0_CTRL
 */
void MPU60X0::setSlaveWriteMode(uint8_t num, bool mode) {
    if (num > 3) return;
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_CTRL + num*3, MPU60X0_I2C_SLV_REG_DIS_BIT, mode);
}
/** Get word pair grouping order offset for the specified slave (0-3).
 * This sets specifies the grouping order of word pairs received from registers.
 * When cleared to 0, bytes from register addresses 0 and 1, 2 and 3, etc (even,
 * then odd register addresses) are paired to form a word. When set to 1, bytes
 * from register addresses are paired 1 and 2, 3 and 4, etc. (odd, then even
 * register addresses) are paired to form a word.
 *
 * @param num Slave number (0-3)
 * @return Current word pair grouping order offset for specified slave
 * @see MPU60X0_RA_I2C_SLV0_CTRL
 */
bool MPU60X0::getSlaveWordGroupOffset(uint8_t num) {
    if (num > 3) return 0;
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_CTRL + num*3, MPU60X0_I2C_SLV_GRP_BIT, buffer);
    return buffer[0];
}
/** Set word pair grouping order offset for the specified slave (0-3).
 * @param num Slave number (0-3)
 * @param enabled New word pair grouping order offset for specified slave
 * @see getSlaveWordGroupOffset()
 * @see MPU60X0_RA_I2C_SLV0_CTRL
 */
void MPU60X0::setSlaveWordGroupOffset(uint8_t num, bool enabled) {
    if (num > 3) return;
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_CTRL + num*3, MPU60X0_I2C_SLV_GRP_BIT, enabled);
}
/** Get number of bytes to read for the specified slave (0-3).
 * Specifies the number of bytes transferred to and from Slave 0. Clearing this
 * bit to 0 is equivalent to disabling the register by writing 0 to I2C_SLV0_EN.
 * @param num Slave number (0-3)
 * @return Number of bytes to read for specified slave
 * @see MPU60X0_RA_I2C_SLV0_CTRL
 */
uint8_t MPU60X0::getSlaveDataLength(uint8_t num) {
    if (num > 3) return 0;
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_CTRL + num*3, MPU60X0_I2C_SLV_LEN_BIT, MPU60X0_I2C_SLV_LEN_LENGTH, buffer);
    return buffer[0];
}
/** Set number of bytes to read for the specified slave (0-3).
 * @param num Slave number (0-3)
 * @param length Number of bytes to read for specified slave
 * @see getSlaveDataLength()
 * @see MPU60X0_RA_I2C_SLV0_CTRL
 */
void MPU60X0::setSlaveDataLength(uint8_t num, uint8_t length) {
    if (num > 3) return;
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_CTRL + num*3, MPU60X0_I2C_SLV_LEN_BIT, MPU60X0_I2C_SLV_LEN_LENGTH, length);
}

// I2C_SLV* registers (Slave 4)

/** Get the I2C address of Slave 4.
 * Note that Bit 7 (MSB) controls read/write mode. If Bit 7 is set, it's a read
 * operation, and if it is cleared, then it's a write operation. The remaining
 * bits (6-0) are the 7-bit device address of the slave device.
 *
 * @return Current address for Slave 4
 * @see getSlaveAddress()
 * @see MPU60X0_RA_I2C_SLV4_ADDR
 */
uint8_t MPU60X0::getSlave4Address() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_ADDR, buffer);
    return buffer[0];
}
/** Set the I2C address of Slave 4.
 * @param address New address for Slave 4
 * @see getSlave4Address()
 * @see MPU60X0_RA_I2C_SLV4_ADDR
 */
void MPU60X0::setSlave4Address(uint8_t address) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_ADDR, address);
}
/** Get the active internal register for the Slave 4.
 * Read/write operations for this slave will be done to whatever internal
 * register address is stored in this MPU register.
 *
 * @return Current active register for Slave 4
 * @see MPU60X0_RA_I2C_SLV4_REG
 */
uint8_t MPU60X0::getSlave4Register() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_REG, buffer);
    return buffer[0];
}
/** Set the active internal register for Slave 4.
 * @param reg New active register for Slave 4
 * @see getSlave4Register()
 * @see MPU60X0_RA_I2C_SLV4_REG
 */
void MPU60X0::setSlave4Register(uint8_t reg) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_REG, reg);
}
/** Set new byte to write to Slave 4.
 * This register stores the data to be written into the Slave 4. If I2C_SLV4_RW
 * is set 1 (set to read), this register has no effect.
 * @param data New byte to write to Slave 4
 * @see MPU60X0_RA_I2C_SLV4_DO
 */
void MPU60X0::setSlave4OutputByte(uint8_t data) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_DO, data);
}
/** Get the enabled value for the Slave 4.
 * When set to 1, this bit enables Slave 4 for data transfer operations. When
 * cleared to 0, this bit disables Slave 4 from data transfer operations.
 * @return Current enabled value for Slave 4
 * @see MPU60X0_RA_I2C_SLV4_CTRL
 */
bool MPU60X0::getSlave4Enabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_CTRL, MPU60X0_I2C_SLV4_EN_BIT, buffer);
    return buffer[0];
}
/** Set the enabled value for Slave 4.
 * @param enabled New enabled value for Slave 4
 * @see getSlave4Enabled()
 * @see MPU60X0_RA_I2C_SLV4_CTRL
 */
void MPU60X0::setSlave4Enabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_CTRL, MPU60X0_I2C_SLV4_EN_BIT, enabled);
}
/** Get the enabled value for Slave 4 transaction interrupts.
 * When set to 1, this bit enables the generation of an interrupt signal upon
 * completion of a Slave 4 transaction. When cleared to 0, this bit disables the
 * generation of an interrupt signal upon completion of a Slave 4 transaction.
 * The interrupt status can be observed in Register 54.
 *
 * @return Current enabled value for Slave 4 transaction interrupts.
 * @see MPU60X0_RA_I2C_SLV4_CTRL
 */
bool MPU60X0::getSlave4InterruptEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_CTRL, MPU60X0_I2C_SLV4_INT_EN_BIT, buffer);
    return buffer[0];
}
/** Set the enabled value for Slave 4 transaction interrupts.
 * @param enabled New enabled value for Slave 4 transaction interrupts.
 * @see getSlave4InterruptEnabled()
 * @see MPU60X0_RA_I2C_SLV4_CTRL
 */
void MPU60X0::setSlave4InterruptEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_CTRL, MPU60X0_I2C_SLV4_INT_EN_BIT, enabled);
}
/** Get write mode for Slave 4.
 * When set to 1, the transaction will read or write data only. When cleared to
 * 0, the transaction will write a register address prior to reading or writing
 * data. This should equal 0 when specifying the register address within the
 * Slave device to/from which the ensuing data transaction will take place.
 *
 * @return Current write mode for Slave 4 (0 = register address + data, 1 = data only)
 * @see MPU60X0_RA_I2C_SLV4_CTRL
 */
bool MPU60X0::getSlave4WriteMode() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_CTRL, MPU60X0_I2C_SLV4_REG_DIS_BIT, buffer);
    return buffer[0];
}
/** Set write mode for the Slave 4.
 * @param mode New write mode for Slave 4 (0 = register address + data, 1 = data only)
 * @see getSlave4WriteMode()
 * @see MPU60X0_RA_I2C_SLV4_CTRL
 */
void MPU60X0::setSlave4WriteMode(bool mode) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_CTRL, MPU60X0_I2C_SLV4_REG_DIS_BIT, mode);
}
/** Get Slave 4 master delay value.
 * This configures the reduced access rate of I2C slaves relative to the Sample
 * Rate. When a slave's access rate is decreased relative to the Sample Rate,
 * the slave is accessed every:
 *
 *     1 / (1 + I2C_MST_DLY) samples
 *
 * This base Sample Rate in turn is determined by SMPLRT_DIV (register 25) and
 * DLPF_CFG (register 26). Whether a slave's access rate is reduced relative to
 * the Sample Rate is determined by I2C_MST_DELAY_CTRL (register 103). For
 * further information regarding the Sample Rate, please refer to register 25.
 *
 * @return Current Slave 4 master delay value
 * @see MPU60X0_RA_I2C_SLV4_CTRL
 */
uint8_t MPU60X0::getSlave4MasterDelay() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_CTRL, MPU60X0_I2C_SLV4_MST_DLY_BIT, MPU60X0_I2C_SLV4_MST_DLY_LENGTH, buffer);
    return buffer[0];
}
/** Set Slave 4 master delay value.
 * @param delay New Slave 4 master delay value
 * @see getSlave4MasterDelay()
 * @see MPU60X0_RA_I2C_SLV4_CTRL
 */
void MPU60X0::setSlave4MasterDelay(uint8_t delay) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_CTRL, MPU60X0_I2C_SLV4_MST_DLY_BIT, MPU60X0_I2C_SLV4_MST_DLY_LENGTH, delay);
}
/** Get last available byte read from Slave 4.
 * This register stores the data read from Slave 4. This field is populated
 * after a read transaction.
 * @return Last available byte read from to Slave 4
 * @see MPU60X0_RA_I2C_SLV4_DI
 */
uint8_t MPU60X0::getSlate4InputByte() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_I2C_SLV4_DI, buffer);
    return buffer[0];
}

// I2C_MST_STATUS register

/** Get FSYNC interrupt status.
 * This bit reflects the status of the FSYNC interrupt from an external device
 * into the MPU-60X0. This is used as a way to pass an external interrupt
 * through the MPU-60X0 to the host application processor. When set to 1, this
 * bit will cause an interrupt if FSYNC_INT_EN is asserted in INT_PIN_CFG
 * (Register 55).
 * @return FSYNC interrupt status
 * @see MPU60X0_RA_I2C_MST_STATUS
 */
bool MPU60X0::getPassthroughStatus() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_STATUS, MPU60X0_MST_PASS_THROUGH_BIT, buffer);
    return buffer[0];
}
/** Get Slave 4 transaction done status.
 * Automatically sets to 1 when a Slave 4 transaction has completed. This
 * triggers an interrupt if the I2C_MST_INT_EN bit in the INT_ENABLE register
 * (Register 56) is asserted and if the SLV_4_DONE_INT bit is asserted in the
 * I2C_SLV4_CTRL register (Register 52).
 * @return Slave 4 transaction done status
 * @see MPU60X0_RA_I2C_MST_STATUS
 */
bool MPU60X0::getSlave4IsDone() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_STATUS, MPU60X0_MST_I2C_SLV4_DONE_BIT, buffer);
    return buffer[0];
}
/** Get master arbitration lost status.
 * This bit automatically sets to 1 when the I2C Master has lost arbitration of
 * the auxiliary I2C bus (an error condition). This triggers an interrupt if the
 * I2C_MST_INT_EN bit in the INT_ENABLE register (Register 56) is asserted.
 * @return Master arbitration lost status
 * @see MPU60X0_RA_I2C_MST_STATUS
 */
bool MPU60X0::getLostArbitration() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_STATUS, MPU60X0_MST_I2C_LOST_ARB_BIT, buffer);
    return buffer[0];
}
/** Get Slave 4 NACK status.
 * This bit automatically sets to 1 when the I2C Master receives a NACK in a
 * transaction with Slave 4. This triggers an interrupt if the I2C_MST_INT_EN
 * bit in the INT_ENABLE register (Register 56) is asserted.
 * @return Slave 4 NACK interrupt status
 * @see MPU60X0_RA_I2C_MST_STATUS
 */
bool MPU60X0::getSlave4Nack() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_STATUS, MPU60X0_MST_I2C_SLV4_NACK_BIT, buffer);
    return buffer[0];
}
/** Get Slave 3 NACK status.
 * This bit automatically sets to 1 when the I2C Master receives a NACK in a
 * transaction with Slave 3. This triggers an interrupt if the I2C_MST_INT_EN
 * bit in the INT_ENABLE register (Register 56) is asserted.
 * @return Slave 3 NACK interrupt status
 * @see MPU60X0_RA_I2C_MST_STATUS
 */
bool MPU60X0::getSlave3Nack() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_STATUS, MPU60X0_MST_I2C_SLV3_NACK_BIT, buffer);
    return buffer[0];
}
/** Get Slave 2 NACK status.
 * This bit automatically sets to 1 when the I2C Master receives a NACK in a
 * transaction with Slave 2. This triggers an interrupt if the I2C_MST_INT_EN
 * bit in the INT_ENABLE register (Register 56) is asserted.
 * @return Slave 2 NACK interrupt status
 * @see MPU60X0_RA_I2C_MST_STATUS
 */
bool MPU60X0::getSlave2Nack() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_STATUS, MPU60X0_MST_I2C_SLV2_NACK_BIT, buffer);
    return buffer[0];
}
/** Get Slave 1 NACK status.
 * This bit automatically sets to 1 when the I2C Master receives a NACK in a
 * transaction with Slave 1. This triggers an interrupt if the I2C_MST_INT_EN
 * bit in the INT_ENABLE register (Register 56) is asserted.
 * @return Slave 1 NACK interrupt status
 * @see MPU60X0_RA_I2C_MST_STATUS
 */
bool MPU60X0::getSlave1Nack() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_STATUS, MPU60X0_MST_I2C_SLV1_NACK_BIT, buffer);
    return buffer[0];
}
/** Get Slave 0 NACK status.
 * This bit automatically sets to 1 when the I2C Master receives a NACK in a
 * transaction with Slave 0. This triggers an interrupt if the I2C_MST_INT_EN
 * bit in the INT_ENABLE register (Register 56) is asserted.
 * @return Slave 0 NACK interrupt status
 * @see MPU60X0_RA_I2C_MST_STATUS
 */
bool MPU60X0::getSlave0Nack() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_STATUS, MPU60X0_MST_I2C_SLV0_NACK_BIT, buffer);
    return buffer[0];
}

// INT_PIN_CFG register

/** Get interrupt logic level mode.
 * Will be set 0 for active-high, 1 for active-low.
 * @return Current interrupt mode (0=active-high, 1=active-low)
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_INT_LEVEL_BIT
 */
bool MPU60X0::getInterruptMode() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_INT_LEVEL_BIT, buffer);
    return buffer[0];
}
/** Set interrupt logic level mode.
 * @param mode New interrupt mode (0=active-high, 1=active-low)
 * @see getInterruptMode()
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_INT_LEVEL_BIT
 */
void MPU60X0::setInterruptMode(bool mode) {
   I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_INT_LEVEL_BIT, mode);
}
/** Get interrupt drive mode.
 * Will be set 0 for push-pull, 1 for open-drain.
 * @return Current interrupt drive mode (0=push-pull, 1=open-drain)
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_INT_OPEN_BIT
 */
bool MPU60X0::getInterruptDrive() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_INT_OPEN_BIT, buffer);
    return buffer[0];
}
/** Set interrupt drive mode.
 * @param drive New interrupt drive mode (0=push-pull, 1=open-drain)
 * @see getInterruptDrive()
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_INT_OPEN_BIT
 */
void MPU60X0::setInterruptDrive(bool drive) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_INT_OPEN_BIT, drive);
}
/** Get interrupt latch mode.
 * Will be set 0 for 50us-pulse, 1 for latch-until-int-cleared.
 * @return Current latch mode (0=50us-pulse, 1=latch-until-int-cleared)
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_LATCH_INT_EN_BIT
 */
bool MPU60X0::getInterruptLatch() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_LATCH_INT_EN_BIT, buffer);
    return buffer[0];
}
/** Set interrupt latch mode.
 * @param latch New latch mode (0=50us-pulse, 1=latch-until-int-cleared)
 * @see getInterruptLatch()
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_LATCH_INT_EN_BIT
 */
void MPU60X0::setInterruptLatch(bool latch) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_LATCH_INT_EN_BIT, latch);
}
/** Get interrupt latch clear mode.
 * Will be set 0 for status-read-only, 1 for any-register-read.
 * @return Current latch clear mode (0=status-read-only, 1=any-register-read)
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_INT_RD_CLEAR_BIT
 */
bool MPU60X0::getInterruptLatchClear() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_INT_RD_CLEAR_BIT, buffer);
    return buffer[0];
}
/** Set interrupt latch clear mode.
 * @param clear New latch clear mode (0=status-read-only, 1=any-register-read)
 * @see getInterruptLatchClear()
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_INT_RD_CLEAR_BIT
 */
void MPU60X0::setInterruptLatchClear(bool clear) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_INT_RD_CLEAR_BIT, clear);
}
/** Get FSYNC interrupt logic level mode.
 * @return Current FSYNC interrupt mode (0=active-high, 1=active-low)
 * @see getFSyncInterruptMode()
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_FSYNC_INT_LEVEL_BIT
 */
bool MPU60X0::getFSyncInterruptLevel() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_FSYNC_INT_LEVEL_BIT, buffer);
    return buffer[0];
}
/** Set FSYNC interrupt logic level mode.
 * @param mode New FSYNC interrupt mode (0=active-high, 1=active-low)
 * @see getFSyncInterruptMode()
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_FSYNC_INT_LEVEL_BIT
 */
void MPU60X0::setFSyncInterruptLevel(bool level) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_FSYNC_INT_LEVEL_BIT, level);
}
/** Get FSYNC pin interrupt enabled setting.
 * Will be set 0 for disabled, 1 for enabled.
 * @return Current interrupt enabled setting
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_FSYNC_INT_EN_BIT
 */
bool MPU60X0::getFSyncInterruptEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_FSYNC_INT_EN_BIT, buffer);
    return buffer[0];
}
/** Set FSYNC pin interrupt enabled setting.
 * @param enabled New FSYNC pin interrupt enabled setting
 * @see getFSyncInterruptEnabled()
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_FSYNC_INT_EN_BIT
 */
void MPU60X0::setFSyncInterruptEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_FSYNC_INT_EN_BIT, enabled);
}
/** Get I2C bypass enabled status.
 * When this bit is equal to 1 and I2C_MST_EN (Register 106 bit[5]) is equal to
 * 0, the host application processor will be able to directly access the
 * auxiliary I2C bus of the MPU-60X0. When this bit is equal to 0, the host
 * application processor will not be able to directly access the auxiliary I2C
 * bus of the MPU-60X0 regardless of the state of I2C_MST_EN (Register 106
 * bit[5]).
 * @return Current I2C bypass enabled status
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_I2C_BYPASS_EN_BIT
 */
bool MPU60X0::getI2CBypassEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_I2C_BYPASS_EN_BIT, buffer);
    return buffer[0];
}
/** Set I2C bypass enabled status.
 * When this bit is equal to 1 and I2C_MST_EN (Register 106 bit[5]) is equal to
 * 0, the host application processor will be able to directly access the
 * auxiliary I2C bus of the MPU-60X0. When this bit is equal to 0, the host
 * application processor will not be able to directly access the auxiliary I2C
 * bus of the MPU-60X0 regardless of the state of I2C_MST_EN (Register 106
 * bit[5]).
 * @param enabled New I2C bypass enabled status
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_I2C_BYPASS_EN_BIT
 */
void MPU60X0::setI2CBypassEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_I2C_BYPASS_EN_BIT, enabled);
}
/** Get reference clock output enabled status.
 * When this bit is equal to 1, a reference clock output is provided at the
 * CLKOUT pin. When this bit is equal to 0, the clock output is disabled. For
 * further information regarding CLKOUT, please refer to the MPU-60X0 Product
 * Specification document.
 * @return Current reference clock output enabled status
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_CLKOUT_EN_BIT
 */
bool MPU60X0::getClockOutputEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_CLKOUT_EN_BIT, buffer);
    return buffer[0];
}
/** Set reference clock output enabled status.
 * When this bit is equal to 1, a reference clock output is provided at the
 * CLKOUT pin. When this bit is equal to 0, the clock output is disabled. For
 * further information regarding CLKOUT, please refer to the MPU-60X0 Product
 * Specification document.
 * @param enabled New reference clock output enabled status
 * @see MPU60X0_RA_INT_PIN_CFG
 * @see MPU60X0_INTCFG_CLKOUT_EN_BIT
 */
void MPU60X0::setClockOutputEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_PIN_CFG, MPU60X0_INTCFG_CLKOUT_EN_BIT, enabled);
}

// INT_ENABLE register
/** Get full interrupt enabled status.
 * Full register byte for all interrupts, for quick reading. Each bit will be
 * set 0 for disabled, 1 for enabled.
 * @return Current interrupt enabled status
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_FF_BIT
 **/
uint8_t MPU60X0::getIntEnabled() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, buffer);
    return buffer[0];
}
/** Set full interrupt enabled status.
 * Full register byte for all interrupts, for quick reading. Each bit should be
 * set 0 for disabled, 1 for enabled.
 * @param enabled New interrupt enabled status
 * @see getIntFreefallEnabled()
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_FF_BIT
 **/
void MPU60X0::setIntEnabled(uint8_t enabled) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, enabled);
}


/** Get Free Fall interrupt enabled status.
 * Will be set 0 for disabled, 1 for enabled.
 * @return Current interrupt enabled status
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_FF_BIT
 **/
bool MPU60X0::getIntFreefallEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_FF_BIT, buffer);
    return buffer[0];
}
/** Set Free Fall interrupt enabled status.
 * @param enabled New interrupt enabled status
 * @see getIntFreefallEnabled()
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_FF_BIT
 **/
void MPU60X0::setIntFreefallEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_FF_BIT, enabled);
}
/** Get Motion Detection interrupt enabled status.
 * Will be set 0 for disabled, 1 for enabled.
 * @return Current interrupt enabled status
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_MOT_BIT
 **/
bool MPU60X0::getIntMotionEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_MOT_BIT, buffer);
    return buffer[0];
}
/** Set Motion Detection interrupt enabled status.
 * @param enabled New interrupt enabled status
 * @see getIntMotionEnabled()
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_MOT_BIT
 **/
void MPU60X0::setIntMotionEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_MOT_BIT, enabled);
}
/** Get Zero Motion Detection interrupt enabled status.
 * Will be set 0 for disabled, 1 for enabled.
 * @return Current interrupt enabled status
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_ZMOT_BIT
 **/
bool MPU60X0::getIntZeroMotionEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_ZMOT_BIT, buffer);
    return buffer[0];
}
/** Set Zero Motion Detection interrupt enabled status.
 * @param enabled New interrupt enabled status
 * @see getIntZeroMotionEnabled()
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_ZMOT_BIT
 **/
void MPU60X0::setIntZeroMotionEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_ZMOT_BIT, enabled);
}
/** Get FIFO Buffer Overflow interrupt enabled status.
 * Will be set 0 for disabled, 1 for enabled.
 * @return Current interrupt enabled status
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_FIFO_OFLOW_BIT
 **/
bool MPU60X0::getIntFIFOBufferOverflowEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_FIFO_OFLOW_BIT, buffer);
    return buffer[0];
}
/** Set FIFO Buffer Overflow interrupt enabled status.
 * @param enabled New interrupt enabled status
 * @see getIntFIFOBufferOverflowEnabled()
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_FIFO_OFLOW_BIT
 **/
void MPU60X0::setIntFIFOBufferOverflowEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_FIFO_OFLOW_BIT, enabled);
}
/** Get I2C Master interrupt enabled status.
 * This enables any of the I2C Master interrupt sources to generate an
 * interrupt. Will be set 0 for disabled, 1 for enabled.
 * @return Current interrupt enabled status
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_I2C_MST_INT_BIT
 **/
bool MPU60X0::getIntI2CMasterEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_I2C_MST_INT_BIT, buffer);
    return buffer[0];
}
/** Set I2C Master interrupt enabled status.
 * @param enabled New interrupt enabled status
 * @see getIntI2CMasterEnabled()
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_I2C_MST_INT_BIT
 **/
void MPU60X0::setIntI2CMasterEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_I2C_MST_INT_BIT, enabled);
}
/** Get Data Ready interrupt enabled setting.
 * This event occurs each time a write operation to all of the sensor registers
 * has been completed. Will be set 0 for disabled, 1 for enabled.
 * @return Current interrupt enabled status
 * @see MPU60X0_RA_INT_ENABLE
 * @see MPU60X0_INTERRUPT_DATA_RDY_BIT
 */
bool MPU60X0::getIntDataReadyEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_DATA_RDY_BIT, buffer);
    return buffer[0];
}
/** Set Data Ready interrupt enabled status.
 * @param enabled New interrupt enabled status
 * @see getIntDataReadyEnabled()
 * @see MPU60X0_RA_INT_CFG
 * @see MPU60X0_INTERRUPT_DATA_RDY_BIT
 */
void MPU60X0::setIntDataReadyEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_DATA_RDY_BIT, enabled);
}

// INT_STATUS register
/** Get full set of interrupt status bits.
 * These bits clear to 0 after the register has been read. Very useful
 * for getting multiple INT statuses, since each single bit read clears
 * all of them because it has to read the whole byte.
 * @return Current interrupt status
 * @see MPU60X0_RA_INT_STATUS
 */
uint8_t MPU60X0::getIntStatus() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_INT_STATUS, buffer);
    return buffer[0];
}
/** Get Free Fall interrupt status.
 * This bit automatically sets to 1 when a Free Fall interrupt has been
 * generated. The bit clears to 0 after the register has been read.
 * @return Current interrupt status
 * @see MPU60X0_RA_INT_STATUS
 * @see MPU60X0_INTERRUPT_FF_BIT
 */
bool MPU60X0::getIntFreefallStatus() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_STATUS, MPU60X0_INTERRUPT_FF_BIT, buffer);
    return buffer[0];
}
/** Get Motion Detection interrupt status.
 * This bit automatically sets to 1 when a Motion Detection interrupt has been
 * generated. The bit clears to 0 after the register has been read.
 * @return Current interrupt status
 * @see MPU60X0_RA_INT_STATUS
 * @see MPU60X0_INTERRUPT_MOT_BIT
 */
bool MPU60X0::getIntMotionStatus() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_STATUS, MPU60X0_INTERRUPT_MOT_BIT, buffer);
    return buffer[0];
}
/** Get Zero Motion Detection interrupt status.
 * This bit automatically sets to 1 when a Zero Motion Detection interrupt has
 * been generated. The bit clears to 0 after the register has been read.
 * @return Current interrupt status
 * @see MPU60X0_RA_INT_STATUS
 * @see MPU60X0_INTERRUPT_ZMOT_BIT
 */
bool MPU60X0::getIntZeroMotionStatus() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_STATUS, MPU60X0_INTERRUPT_ZMOT_BIT, buffer);
    return buffer[0];
}
/** Get FIFO Buffer Overflow interrupt status.
 * This bit automatically sets to 1 when a Free Fall interrupt has been
 * generated. The bit clears to 0 after the register has been read.
 * @return Current interrupt status
 * @see MPU60X0_RA_INT_STATUS
 * @see MPU60X0_INTERRUPT_FIFO_OFLOW_BIT
 */
bool MPU60X0::getIntFIFOBufferOverflowStatus() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_STATUS, MPU60X0_INTERRUPT_FIFO_OFLOW_BIT, buffer);
    return buffer[0];
}
/** Get I2C Master interrupt status.
 * This bit automatically sets to 1 when an I2C Master interrupt has been
 * generated. For a list of I2C Master interrupts, please refer to Register 54.
 * The bit clears to 0 after the register has been read.
 * @return Current interrupt status
 * @see MPU60X0_RA_INT_STATUS
 * @see MPU60X0_INTERRUPT_I2C_MST_INT_BIT
 */
bool MPU60X0::getIntI2CMasterStatus() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_STATUS, MPU60X0_INTERRUPT_I2C_MST_INT_BIT, buffer);
    return buffer[0];
}
/** Get Data Ready interrupt status.
 * This bit automatically sets to 1 when a Data Ready interrupt has been
 * generated. The bit clears to 0 after the register has been read.
 * @return Current interrupt status
 * @see MPU60X0_RA_INT_STATUS
 * @see MPU60X0_INTERRUPT_DATA_RDY_BIT
 */
bool MPU60X0::getIntDataReadyStatus() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_STATUS, MPU60X0_INTERRUPT_DATA_RDY_BIT, buffer);
    return buffer[0];
}

// ACCEL_*OUT_* registers

/** Get raw 9-axis motion sensor readings (accel/gyro/compass).
 * FUNCTION NOT FULLY IMPLEMENTED YET.
 * @param ax 16-bit signed integer container for accelerometer X-axis value
 * @param ay 16-bit signed integer container for accelerometer Y-axis value
 * @param az 16-bit signed integer container for accelerometer Z-axis value
 * @param gx 16-bit signed integer container for gyroscope X-axis value
 * @param gy 16-bit signed integer container for gyroscope Y-axis value
 * @param gz 16-bit signed integer container for gyroscope Z-axis value
 * @param mx 16-bit signed integer container for magnetometer X-axis value
 * @param my 16-bit signed integer container for magnetometer Y-axis value
 * @param mz 16-bit signed integer container for magnetometer Z-axis value
 * @see getMotion6()
 * @see getAcceleration()
 * @see getRotation()
 * @see MPU60X0_RA_ACCEL_XOUT_H
 */
void MPU60X0::getMotion9(int16_t* ax, int16_t* ay, int16_t* az, int16_t* gx, int16_t* gy, int16_t* gz, int16_t* mx, int16_t* my, int16_t* mz) {
    getMotion6(ax, ay, az, gx, gy, gz);
    // TODO: magnetometer integration
}
/** Get raw 6-axis motion sensor readings (accel/gyro).
 * Retrieves all currently available motion sensor values.
 * @param ax 16-bit signed integer container for accelerometer X-axis value
 * @param ay 16-bit signed integer container for accelerometer Y-axis value
 * @param az 16-bit signed integer container for accelerometer Z-axis value
 * @param gx 16-bit signed integer container for gyroscope X-axis value
 * @param gy 16-bit signed integer container for gyroscope Y-axis value
 * @param gz 16-bit signed integer container for gyroscope Z-axis value
 * @see getAcceleration()
 * @see getRotation()
 * @see MPU60X0_RA_ACCEL_XOUT_H
 */
void MPU60X0::getMotion6(int16_t* ax, int16_t* ay, int16_t* az, int16_t* gx, int16_t* gy, int16_t* gz) {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_ACCEL_XOUT_H, 14, buffer);
    *ax = (((int16_t)buffer[0]) << 8) | buffer[1];
    *ay = (((int16_t)buffer[2]) << 8) | buffer[3];
    *az = (((int16_t)buffer[4]) << 8) | buffer[5];
    *gx = (((int16_t)buffer[8]) << 8) | buffer[9];
    *gy = (((int16_t)buffer[10]) << 8) | buffer[11];
    *gz = (((int16_t)buffer[12]) << 8) | buffer[13];
}
/** Get 3-axis accelerometer readings.
 * These registers store the most recent accelerometer measurements.
 * Accelerometer measurements are written to these registers at the Sample Rate
 * as defined in Register 25.
 *
 * The accelerometer measurement registers, along with the temperature
 * measurement registers, gyroscope measurement registers, and external sensor
 * data registers, are composed of two sets of registers: an internal register
 * set and a user-facing read register set.
 *
 * The data within the accelerometer sensors' internal register set is always
 * updated at the Sample Rate. Meanwhile, the user-facing read register set
 * duplicates the internal register set's data values whenever the serial
 * interface is idle. This guarantees that a burst read of sensor registers will
 * read measurements from the same sampling instant. Note that if burst reads
 * are not used, the user is responsible for ensuring a set of single byte reads
 * correspond to a single sampling instant by checking the Data Ready interrupt.
 *
 * Each 16-bit accelerometer measurement has a full scale defined in ACCEL_FS
 * (Register 28). For each full scale setting, the accelerometers' sensitivity
 * per LSB in ACCEL_xOUT is shown in the table below:
 *
 * <pre>
 * AFS_SEL | Full Scale Range | LSB Sensitivity
 * --------+------------------+----------------
 * 0       | +/- 2g           | 8192 LSB/mg
 * 1       | +/- 4g           | 4096 LSB/mg
 * 2       | +/- 8g           | 2048 LSB/mg
 * 3       | +/- 16g          | 1024 LSB/mg
 * </pre>
 *
 * @param x 16-bit signed integer container for X-axis acceleration
 * @param y 16-bit signed integer container for Y-axis acceleration
 * @param z 16-bit signed integer container for Z-axis acceleration
 * @see MPU60X0_RA_GYRO_XOUT_H
 */
void MPU60X0::getAcceleration(int16_t* x, int16_t* y, int16_t* z) {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_ACCEL_XOUT_H, 6, buffer);
    *x = (((int16_t)buffer[0]) << 8) | buffer[1];
    *y = (((int16_t)buffer[2]) << 8) | buffer[3];
    *z = (((int16_t)buffer[4]) << 8) | buffer[5];
}
/** Get X-axis accelerometer reading.
 * @return X-axis acceleration measurement in 16-bit 2's complement format
 * @see getMotion6()
 * @see MPU60X0_RA_ACCEL_XOUT_H
 */
int16_t MPU60X0::getAccelerationX() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_ACCEL_XOUT_H, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
/** Get Y-axis accelerometer reading.
 * @return Y-axis acceleration measurement in 16-bit 2's complement format
 * @see getMotion6()
 * @see MPU60X0_RA_ACCEL_YOUT_H
 */
int16_t MPU60X0::getAccelerationY() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_ACCEL_YOUT_H, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
/** Get Z-axis accelerometer reading.
 * @return Z-axis acceleration measurement in 16-bit 2's complement format
 * @see getMotion6()
 * @see MPU60X0_RA_ACCEL_ZOUT_H
 */
int16_t MPU60X0::getAccelerationZ() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_ACCEL_ZOUT_H, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}

// TEMP_OUT_* registers

/** Get current internal temperature.
 * @return Temperature reading in 16-bit 2's complement format
 * @see MPU60X0_RA_TEMP_OUT_H
 */
int16_t MPU60X0::getTemperature() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_TEMP_OUT_H, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}

// GYRO_*OUT_* registers

/** Get 3-axis gyroscope readings.
 * These gyroscope measurement registers, along with the accelerometer
 * measurement registers, temperature measurement registers, and external sensor
 * data registers, are composed of two sets of registers: an internal register
 * set and a user-facing read register set.
 * The data within the gyroscope sensors' internal register set is always
 * updated at the Sample Rate. Meanwhile, the user-facing read register set
 * duplicates the internal register set's data values whenever the serial
 * interface is idle. This guarantees that a burst read of sensor registers will
 * read measurements from the same sampling instant. Note that if burst reads
 * are not used, the user is responsible for ensuring a set of single byte reads
 * correspond to a single sampling instant by checking the Data Ready interrupt.
 *
 * Each 16-bit gyroscope measurement has a full scale defined in FS_SEL
 * (Register 27). For each full scale setting, the gyroscopes' sensitivity per
 * LSB in GYRO_xOUT is shown in the table below:
 *
 * <pre>
 * FS_SEL | Full Scale Range   | LSB Sensitivity
 * -------+--------------------+----------------
 * 0      | +/- 250 degrees/s  | 131 LSB/deg/s
 * 1      | +/- 500 degrees/s  | 65.5 LSB/deg/s
 * 2      | +/- 1000 degrees/s | 32.8 LSB/deg/s
 * 3      | +/- 2000 degrees/s | 16.4 LSB/deg/s
 * </pre>
 *
 * @param x 16-bit signed integer container for X-axis rotation
 * @param y 16-bit signed integer container for Y-axis rotation
 * @param z 16-bit signed integer container for Z-axis rotation
 * @see getMotion6()
 * @see MPU60X0_RA_GYRO_XOUT_H
 */
void MPU60X0::getRotation(int16_t* x, int16_t* y, int16_t* z) {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_GYRO_XOUT_H, 6, buffer);
    *x = (((int16_t)buffer[0]) << 8) | buffer[1];
    *y = (((int16_t)buffer[2]) << 8) | buffer[3];
    *z = (((int16_t)buffer[4]) << 8) | buffer[5];
}
/** Get X-axis gyroscope reading.
 * @return X-axis rotation measurement in 16-bit 2's complement format
 * @see getMotion6()
 * @see MPU60X0_RA_GYRO_XOUT_H
 */
int16_t MPU60X0::getRotationX() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_GYRO_XOUT_H, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
/** Get Y-axis gyroscope reading.
 * @return Y-axis rotation measurement in 16-bit 2's complement format
 * @see getMotion6()
 * @see MPU60X0_RA_GYRO_YOUT_H
 */
int16_t MPU60X0::getRotationY() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_GYRO_YOUT_H, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
/** Get Z-axis gyroscope reading.
 * @return Z-axis rotation measurement in 16-bit 2's complement format
 * @see getMotion6()
 * @see MPU60X0_RA_GYRO_ZOUT_H
 */
int16_t MPU60X0::getRotationZ() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_GYRO_ZOUT_H, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}

// EXT_SENS_DATA_* registers

/** Read single byte from external sensor data register.
 * These registers store data read from external sensors by the Slave 0, 1, 2,
 * and 3 on the auxiliary I2C interface. Data read by Slave 4 is stored in
 * I2C_SLV4_DI (Register 53).
 *
 * External sensor data is written to these registers at the Sample Rate as
 * defined in Register 25. This access rate can be reduced by using the Slave
 * Delay Enable registers (Register 103).
 *
 * External sensor data registers, along with the gyroscope measurement
 * registers, accelerometer measurement registers, and temperature measurement
 * registers, are composed of two sets of registers: an internal register set
 * and a user-facing read register set.
 *
 * The data within the external sensors' internal register set is always updated
 * at the Sample Rate (or the reduced access rate) whenever the serial interface
 * is idle. This guarantees that a burst read of sensor registers will read
 * measurements from the same sampling instant. Note that if burst reads are not
 * used, the user is responsible for ensuring a set of single byte reads
 * correspond to a single sampling instant by checking the Data Ready interrupt.
 *
 * Data is placed in these external sensor data registers according to
 * I2C_SLV0_CTRL, I2C_SLV1_CTRL, I2C_SLV2_CTRL, and I2C_SLV3_CTRL (Registers 39,
 * 42, 45, and 48). When more than zero bytes are read (I2C_SLVx_LEN > 0) from
 * an enabled slave (I2C_SLVx_EN = 1), the slave is read at the Sample Rate (as
 * defined in Register 25) or delayed rate (if specified in Register 52 and
 * 103). During each Sample cycle, slave reads are performed in order of Slave
 * number. If all slaves are enabled with more than zero bytes to be read, the
 * order will be Slave 0, followed by Slave 1, Slave 2, and Slave 3.
 *
 * Each enabled slave will have EXT_SENS_DATA registers associated with it by
 * number of bytes read (I2C_SLVx_LEN) in order of slave number, starting from
 * EXT_SENS_DATA_00. Note that this means enabling or disabling a slave may
 * change the higher numbered slaves' associated registers. Furthermore, if
 * fewer total bytes are being read from the external sensors as a result of
 * such a change, then the data remaining in the registers which no longer have
 * an associated slave device (i.e. high numbered registers) will remain in
 * these previously allocated registers unless reset.
 *
 * If the sum of the read lengths of all SLVx transactions exceed the number of
 * available EXT_SENS_DATA registers, the excess bytes will be dropped. There
 * are 24 EXT_SENS_DATA registers and hence the total read lengths between all
 * the slaves cannot be greater than 24 or some bytes will be lost.
 *
 * Note: Slave 4's behavior is distinct from that of Slaves 0-3. For further
 * information regarding the characteristics of Slave 4, please refer to
 * Registers 49 to 53.
 *
 * EXAMPLE:
 * Suppose that Slave 0 is enabled with 4 bytes to be read (I2C_SLV0_EN = 1 and
 * I2C_SLV0_LEN = 4) while Slave 1 is enabled with 2 bytes to be read so that
 * I2C_SLV1_EN = 1 and I2C_SLV1_LEN = 2. In such a situation, EXT_SENS_DATA _00
 * through _03 will be associated with Slave 0, while EXT_SENS_DATA _04 and 05
 * will be associated with Slave 1. If Slave 2 is enabled as well, registers
 * starting from EXT_SENS_DATA_06 will be allocated to Slave 2.
 *
 * If Slave 2 is disabled while Slave 3 is enabled in this same situation, then
 * registers starting from EXT_SENS_DATA_06 will be allocated to Slave 3
 * instead.
 *
 * REGISTER ALLOCATION FOR DYNAMIC DISABLE VS. NORMAL DISABLE:
 * If a slave is disabled at any time, the space initially allocated to the
 * slave in the EXT_SENS_DATA register, will remain associated with that slave.
 * This is to avoid dynamic adjustment of the register allocation.
 *
 * The allocation of the EXT_SENS_DATA registers is recomputed only when (1) all
 * slaves are disabled, or (2) the I2C_MST_RST bit is set (Register 106).
 *
 * This above is also true if one of the slaves gets NACKed and stops
 * functioning.
 *
 * @param position Starting position (0-23)
 * @return Byte read from register
 */
uint8_t MPU60X0::getExternalSensorByte(int position) {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_EXT_SENS_DATA_00 + position, buffer);
    return buffer[0];
}
/** Read word (2 bytes) from external sensor data registers.
 * @param position Starting position (0-21)
 * @return Word read from register
 * @see getExternalSensorByte()
 */
uint16_t MPU60X0::getExternalSensorWord(int position) {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_EXT_SENS_DATA_00 + position, 2, buffer);
    return (((uint16_t)buffer[0]) << 8) | buffer[1];
}
/** Read double word (4 bytes) from external sensor data registers.
 * @param position Starting position (0-20)
 * @return Double word read from registers
 * @see getExternalSensorByte()
 */
uint32_t MPU60X0::getExternalSensorDWord(int position) {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_EXT_SENS_DATA_00 + position, 4, buffer);
    return (((uint32_t)buffer[0]) << 24) | (((uint32_t)buffer[1]) << 16) | (((uint16_t)buffer[2]) << 8) | buffer[3];
}

// MOT_DETECT_STATUS register

/** Get X-axis negative motion detection interrupt status.
 * @return Motion detection status
 * @see MPU60X0_RA_MOT_DETECT_STATUS
 * @see MPU60X0_MOTION_MOT_XNEG_BIT
 */
bool MPU60X0::getXNegMotionDetected() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_STATUS, MPU60X0_MOTION_MOT_XNEG_BIT, buffer);
    return buffer[0];
}
/** Get X-axis positive motion detection interrupt status.
 * @return Motion detection status
 * @see MPU60X0_RA_MOT_DETECT_STATUS
 * @see MPU60X0_MOTION_MOT_XPOS_BIT
 */
bool MPU60X0::getXPosMotionDetected() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_STATUS, MPU60X0_MOTION_MOT_XPOS_BIT, buffer);
    return buffer[0];
}
/** Get Y-axis negative motion detection interrupt status.
 * @return Motion detection status
 * @see MPU60X0_RA_MOT_DETECT_STATUS
 * @see MPU60X0_MOTION_MOT_YNEG_BIT
 */
bool MPU60X0::getYNegMotionDetected() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_STATUS, MPU60X0_MOTION_MOT_YNEG_BIT, buffer);
    return buffer[0];
}
/** Get Y-axis positive motion detection interrupt status.
 * @return Motion detection status
 * @see MPU60X0_RA_MOT_DETECT_STATUS
 * @see MPU60X0_MOTION_MOT_YPOS_BIT
 */
bool MPU60X0::getYPosMotionDetected() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_STATUS, MPU60X0_MOTION_MOT_YPOS_BIT, buffer);
    return buffer[0];
}
/** Get Z-axis negative motion detection interrupt status.
 * @return Motion detection status
 * @see MPU60X0_RA_MOT_DETECT_STATUS
 * @see MPU60X0_MOTION_MOT_ZNEG_BIT
 */
bool MPU60X0::getZNegMotionDetected() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_STATUS, MPU60X0_MOTION_MOT_ZNEG_BIT, buffer);
    return buffer[0];
}
/** Get Z-axis positive motion detection interrupt status.
 * @return Motion detection status
 * @see MPU60X0_RA_MOT_DETECT_STATUS
 * @see MPU60X0_MOTION_MOT_ZPOS_BIT
 */
bool MPU60X0::getZPosMotionDetected() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_STATUS, MPU60X0_MOTION_MOT_ZPOS_BIT, buffer);
    return buffer[0];
}
/** Get zero motion detection interrupt status.
 * @return Motion detection status
 * @see MPU60X0_RA_MOT_DETECT_STATUS
 * @see MPU60X0_MOTION_MOT_ZRMOT_BIT
 */
bool MPU60X0::getZeroMotionDetected() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_STATUS, MPU60X0_MOTION_MOT_ZRMOT_BIT, buffer);
    return buffer[0];
}

// I2C_SLV*_DO register

/** Write byte to Data Output container for specified slave.
 * This register holds the output data written into Slave when Slave is set to
 * write mode. For further information regarding Slave control, please
 * refer to Registers 37 to 39 and immediately following.
 * @param num Slave number (0-3)
 * @param data Byte to write
 * @see MPU60X0_RA_I2C_SLV0_DO
 */
void MPU60X0::setSlaveOutputByte(uint8_t num, uint8_t data) {
    if (num > 3) return;
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_I2C_SLV0_DO + num, data);
}

// I2C_MST_DELAY_CTRL register

/** Get external data shadow delay enabled status.
 * This register is used to specify the timing of external sensor data
 * shadowing. When DELAY_ES_SHADOW is set to 1, shadowing of external
 * sensor data is delayed until all data has been received.
 * @return Current external data shadow delay enabled status.
 * @see MPU60X0_RA_I2C_MST_DELAY_CTRL
 * @see MPU60X0_DELAYCTRL_DELAY_ES_SHADOW_BIT
 */
bool MPU60X0::getExternalShadowDelayEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_DELAY_CTRL, MPU60X0_DELAYCTRL_DELAY_ES_SHADOW_BIT, buffer);
    return buffer[0];
}
/** Set external data shadow delay enabled status.
 * @param enabled New external data shadow delay enabled status.
 * @see getExternalShadowDelayEnabled()
 * @see MPU60X0_RA_I2C_MST_DELAY_CTRL
 * @see MPU60X0_DELAYCTRL_DELAY_ES_SHADOW_BIT
 */
void MPU60X0::setExternalShadowDelayEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_DELAY_CTRL, MPU60X0_DELAYCTRL_DELAY_ES_SHADOW_BIT, enabled);
}
/** Get slave delay enabled status.
 * When a particular slave delay is enabled, the rate of access for the that
 * slave device is reduced. When a slave's access rate is decreased relative to
 * the Sample Rate, the slave is accessed every:
 *
 *     1 / (1 + I2C_MST_DLY) Samples
 *
 * This base Sample Rate in turn is determined by SMPLRT_DIV (register  * 25)
 * and DLPF_CFG (register 26).
 *
 * For further information regarding I2C_MST_DLY, please refer to register 52.
 * For further information regarding the Sample Rate, please refer to register 25.
 *
 * @param num Slave number (0-4)
 * @return Current slave delay enabled status.
 * @see MPU60X0_RA_I2C_MST_DELAY_CTRL
 * @see MPU60X0_DELAYCTRL_I2C_SLV0_DLY_EN_BIT
 */
bool MPU60X0::getSlaveDelayEnabled(uint8_t num) {
    // MPU60X0_DELAYCTRL_I2C_SLV4_DLY_EN_BIT is 4, SLV3 is 3, etc.
    if (num > 4) return 0;
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_DELAY_CTRL, num, buffer);
    return buffer[0];
}
/** Set slave delay enabled status.
 * @param num Slave number (0-4)
 * @param enabled New slave delay enabled status.
 * @see MPU60X0_RA_I2C_MST_DELAY_CTRL
 * @see MPU60X0_DELAYCTRL_I2C_SLV0_DLY_EN_BIT
 */
void MPU60X0::setSlaveDelayEnabled(uint8_t num, bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_I2C_MST_DELAY_CTRL, num, enabled);
}

// SIGNAL_PATH_RESET register

/** Reset gyroscope signal path.
 * The reset will revert the signal path analog to digital converters and
 * filters to their power up configurations.
 * @see MPU60X0_RA_SIGNAL_PATH_RESET
 * @see MPU60X0_PATHRESET_GYRO_RESET_BIT
 */
void MPU60X0::resetGyroscopePath() {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_SIGNAL_PATH_RESET, MPU60X0_PATHRESET_GYRO_RESET_BIT, true);
}
/** Reset accelerometer signal path.
 * The reset will revert the signal path analog to digital converters and
 * filters to their power up configurations.
 * @see MPU60X0_RA_SIGNAL_PATH_RESET
 * @see MPU60X0_PATHRESET_ACCEL_RESET_BIT
 */
void MPU60X0::resetAccelerometerPath() {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_SIGNAL_PATH_RESET, MPU60X0_PATHRESET_ACCEL_RESET_BIT, true);
}
/** Reset temperature sensor signal path.
 * The reset will revert the signal path analog to digital converters and
 * filters to their power up configurations.
 * @see MPU60X0_RA_SIGNAL_PATH_RESET
 * @see MPU60X0_PATHRESET_TEMP_RESET_BIT
 */
void MPU60X0::resetTemperaturePath() {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_SIGNAL_PATH_RESET, MPU60X0_PATHRESET_TEMP_RESET_BIT, true);
}

// MOT_DETECT_CTRL register

/** Get accelerometer power-on delay.
 * The accelerometer data path provides samples to the sensor registers, Motion
 * detection, Zero Motion detection, and Free Fall detection modules. The
 * signal path contains filters which must be flushed on wake-up with new
 * samples before the detection modules begin operations. The default wake-up
 * delay, of 4ms can be lengthened by up to 3ms. This additional delay is
 * specified in ACCEL_ON_DELAY in units of 1 LSB = 1 ms. The user may select
 * any value above zero unless instructed otherwise by InvenSense. Please refer
 * to Section 8 of the MPU-6000/MPU-6050 Product Specification document for
 * further information regarding the detection modules.
 * @return Current accelerometer power-on delay
 * @see MPU60X0_RA_MOT_DETECT_CTRL
 * @see MPU60X0_DETECT_ACCEL_ON_DELAY_BIT
 */
uint8_t MPU60X0::getAccelerometerPowerOnDelay() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_CTRL, MPU60X0_DETECT_ACCEL_ON_DELAY_BIT, MPU60X0_DETECT_ACCEL_ON_DELAY_LENGTH, buffer);
    return buffer[0];
}
/** Set accelerometer power-on delay.
 * @param delay New accelerometer power-on delay (0-3)
 * @see getAccelerometerPowerOnDelay()
 * @see MPU60X0_RA_MOT_DETECT_CTRL
 * @see MPU60X0_DETECT_ACCEL_ON_DELAY_BIT
 */
void MPU60X0::setAccelerometerPowerOnDelay(uint8_t delay) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_CTRL, MPU60X0_DETECT_ACCEL_ON_DELAY_BIT, MPU60X0_DETECT_ACCEL_ON_DELAY_LENGTH, delay);
}
/** Get Free Fall detection counter decrement configuration.
 * Detection is registered by the Free Fall detection module after accelerometer
 * measurements meet their respective threshold conditions over a specified
 * number of samples. When the threshold conditions are met, the corresponding
 * detection counter increments by 1. The user may control the rate at which the
 * detection counter decrements when the threshold condition is not met by
 * configuring FF_COUNT. The decrement rate can be set according to the
 * following table:
 *
 * <pre>
 * FF_COUNT | Counter Decrement
 * ---------+------------------
 * 0        | Reset
 * 1        | 1
 * 2        | 2
 * 3        | 4
 * </pre>
 *
 * When FF_COUNT is configured to 0 (reset), any non-qualifying sample will
 * reset the counter to 0. For further information on Free Fall detection,
 * please refer to Registers 29 to 32.
 *
 * @return Current decrement configuration
 * @see MPU60X0_RA_MOT_DETECT_CTRL
 * @see MPU60X0_DETECT_FF_COUNT_BIT
 */
uint8_t MPU60X0::getFreefallDetectionCounterDecrement() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_CTRL, MPU60X0_DETECT_FF_COUNT_BIT, MPU60X0_DETECT_FF_COUNT_LENGTH, buffer);
    return buffer[0];
}
/** Set Free Fall detection counter decrement configuration.
 * @param decrement New decrement configuration value
 * @see getFreefallDetectionCounterDecrement()
 * @see MPU60X0_RA_MOT_DETECT_CTRL
 * @see MPU60X0_DETECT_FF_COUNT_BIT
 */
void MPU60X0::setFreefallDetectionCounterDecrement(uint8_t decrement) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_CTRL, MPU60X0_DETECT_FF_COUNT_BIT, MPU60X0_DETECT_FF_COUNT_LENGTH, decrement);
}
/** Get Motion detection counter decrement configuration.
 * Detection is registered by the Motion detection module after accelerometer
 * measurements meet their respective threshold conditions over a specified
 * number of samples. When the threshold conditions are met, the corresponding
 * detection counter increments by 1. The user may control the rate at which the
 * detection counter decrements when the threshold condition is not met by
 * configuring MOT_COUNT. The decrement rate can be set according to the
 * following table:
 *
 * <pre>
 * MOT_COUNT | Counter Decrement
 * ----------+------------------
 * 0         | Reset
 * 1         | 1
 * 2         | 2
 * 3         | 4
 * </pre>
 *
 * When MOT_COUNT is configured to 0 (reset), any non-qualifying sample will
 * reset the counter to 0. For further information on Motion detection,
 * please refer to Registers 29 to 32.
 *
 */
uint8_t MPU60X0::getMotionDetectionCounterDecrement() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_CTRL, MPU60X0_DETECT_MOT_COUNT_BIT, MPU60X0_DETECT_MOT_COUNT_LENGTH, buffer);
    return buffer[0];
}
/** Set Motion detection counter decrement configuration.
 * @param decrement New decrement configuration value
 * @see getMotionDetectionCounterDecrement()
 * @see MPU60X0_RA_MOT_DETECT_CTRL
 * @see MPU60X0_DETECT_MOT_COUNT_BIT
 */
void MPU60X0::setMotionDetectionCounterDecrement(uint8_t decrement) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_MOT_DETECT_CTRL, MPU60X0_DETECT_MOT_COUNT_BIT, MPU60X0_DETECT_MOT_COUNT_LENGTH, decrement);
}

// USER_CTRL register

/** Get FIFO enabled status.
 * When this bit is set to 0, the FIFO buffer is disabled. The FIFO buffer
 * cannot be written to or read from while disabled. The FIFO buffer's state
 * does not change unless the MPU-60X0 is power cycled.
 * @return Current FIFO enabled status
 * @see MPU60X0_RA_USER_CTRL
 * @see MPU60X0_USERCTRL_FIFO_EN_BIT
 */
bool MPU60X0::getFIFOEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_USER_CTRL, MPU60X0_USERCTRL_FIFO_EN_BIT, buffer);
    return buffer[0];
}
/** Set FIFO enabled status.
 * @param enabled New FIFO enabled status
 * @see getFIFOEnabled()
 * @see MPU60X0_RA_USER_CTRL
 * @see MPU60X0_USERCTRL_FIFO_EN_BIT
 */
void MPU60X0::setFIFOEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_USER_CTRL, MPU60X0_USERCTRL_FIFO_EN_BIT, enabled);
}
/** Get I2C Master Mode enabled status.
 * When this mode is enabled, the MPU-60X0 acts as the I2C Master to the
 * external sensor slave devices on the auxiliary I2C bus. When this bit is
 * cleared to 0, the auxiliary I2C bus lines (AUX_DA and AUX_CL) are logically
 * driven by the primary I2C bus (SDA and SCL). This is a precondition to
 * enabling Bypass Mode. For further information regarding Bypass Mode, please
 * refer to Register 55.
 * @return Current I2C Master Mode enabled status
 * @see MPU60X0_RA_USER_CTRL
 * @see MPU60X0_USERCTRL_I2C_MST_EN_BIT
 */
bool MPU60X0::getI2CMasterModeEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_USER_CTRL, MPU60X0_USERCTRL_I2C_MST_EN_BIT, buffer);
    return buffer[0];
}
/** Set I2C Master Mode enabled status.
 * @param enabled New I2C Master Mode enabled status
 * @see getI2CMasterModeEnabled()
 * @see MPU60X0_RA_USER_CTRL
 * @see MPU60X0_USERCTRL_I2C_MST_EN_BIT
 */
void MPU60X0::setI2CMasterModeEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_USER_CTRL, MPU60X0_USERCTRL_I2C_MST_EN_BIT, enabled);
}
/** Switch from I2C to SPI mode (MPU-6000 only)
 * If this is set, the primary SPI interface will be enabled in place of the
 * disabled primary I2C interface.
 */
void MPU60X0::switchSPIEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_USER_CTRL, MPU60X0_USERCTRL_I2C_IF_DIS_BIT, enabled);
}
/** Reset the FIFO.
 * This bit resets the FIFO buffer when set to 1 while FIFO_EN equals 0. This
 * bit automatically clears to 0 after the reset has been triggered.
 * @see MPU60X0_RA_USER_CTRL
 * @see MPU60X0_USERCTRL_FIFO_RESET_BIT
 */
void MPU60X0::resetFIFO() {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_USER_CTRL, MPU60X0_USERCTRL_FIFO_RESET_BIT, true);
}
/** Reset the I2C Master.
 * This bit resets the I2C Master when set to 1 while I2C_MST_EN equals 0.
 * This bit automatically clears to 0 after the reset has been triggered.
 * @see MPU60X0_RA_USER_CTRL
 * @see MPU60X0_USERCTRL_I2C_MST_RESET_BIT
 */
void MPU60X0::resetI2CMaster() {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_USER_CTRL, MPU60X0_USERCTRL_I2C_MST_RESET_BIT, true);
}
/** Reset all sensor registers and signal paths.
 * When set to 1, this bit resets the signal paths for all sensors (gyroscopes,
 * accelerometers, and temperature sensor). This operation will also clear the
 * sensor registers. This bit automatically clears to 0 after the reset has been
 * triggered.
 *
 * When resetting only the signal path (and not the sensor registers), please
 * use Register 104, SIGNAL_PATH_RESET.
 *
 * @see MPU60X0_RA_USER_CTRL
 * @see MPU60X0_USERCTRL_SIG_COND_RESET_BIT
 */
void MPU60X0::resetSensors() {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_USER_CTRL, MPU60X0_USERCTRL_SIG_COND_RESET_BIT, true);
}

// PWR_MGMT_1 register

/** Trigger a full device reset.
 * A small delay of ~50ms may be desirable after triggering a reset.
 * @see MPU60X0_RA_PWR_MGMT_1
 * @see MPU60X0_PWR1_DEVICE_RESET_BIT
 */
void MPU60X0::reset() {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_1, MPU60X0_PWR1_DEVICE_RESET_BIT, true);
	delay(50);
}
/** Get sleep mode status.
 * Setting the SLEEP bit in the register puts the device into very low power
 * sleep mode. In this mode, only the serial interface and internal registers
 * remain active, allowing for a very low standby current. Clearing this bit
 * puts the device back into normal mode. To save power, the individual standby
 * selections for each of the gyros should be used if any gyro axis is not used
 * by the application.
 * @return Current sleep mode enabled status
 * @see MPU60X0_RA_PWR_MGMT_1
 * @see MPU60X0_PWR1_SLEEP_BIT
 */
bool MPU60X0::getSleepEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_1, MPU60X0_PWR1_SLEEP_BIT, buffer);
    return buffer[0];
}
/** Set sleep mode status.
 * @param enabled New sleep mode enabled status
 * @see getSleepEnabled()
 * @see MPU60X0_RA_PWR_MGMT_1
 * @see MPU60X0_PWR1_SLEEP_BIT
 */
void MPU60X0::setSleepEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_1, MPU60X0_PWR1_SLEEP_BIT, enabled);
}
/** Get wake cycle enabled status.
 * When this bit is set to 1 and SLEEP is disabled, the MPU-60X0 will cycle
 * between sleep mode and waking up to take a single sample of data from active
 * sensors at a rate determined by LP_WAKE_CTRL (register 108).
 * @return Current sleep mode enabled status
 * @see MPU60X0_RA_PWR_MGMT_1
 * @see MPU60X0_PWR1_CYCLE_BIT
 */
bool MPU60X0::getWakeCycleEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_1, MPU60X0_PWR1_CYCLE_BIT, buffer);
    return buffer[0];
}
/** Set wake cycle enabled status.
 * @param enabled New sleep mode enabled status
 * @see getWakeCycleEnabled()
 * @see MPU60X0_RA_PWR_MGMT_1
 * @see MPU60X0_PWR1_CYCLE_BIT
 */
void MPU60X0::setWakeCycleEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_1, MPU60X0_PWR1_CYCLE_BIT, enabled);
}
/** Get temperature sensor enabled status.
 * Control the usage of the internal temperature sensor.
 *
 * Note: this register stores the *disabled* value, but for consistency with the
 * rest of the code, the function is named and used with standard true/false
 * values to indicate whether the sensor is enabled or disabled, respectively.
 *
 * @return Current temperature sensor enabled status
 * @see MPU60X0_RA_PWR_MGMT_1
 * @see MPU60X0_PWR1_TEMP_DIS_BIT
 */
bool MPU60X0::getTempSensorEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_1, MPU60X0_PWR1_TEMP_DIS_BIT, buffer);
    return buffer[0] == 0; // 1 is actually disabled here
}
/** Set temperature sensor enabled status.
 * Note: this register stores the *disabled* value, but for consistency with the
 * rest of the code, the function is named and used with standard true/false
 * values to indicate whether the sensor is enabled or disabled, respectively.
 *
 * @param enabled New temperature sensor enabled status
 * @see getTempSensorEnabled()
 * @see MPU60X0_RA_PWR_MGMT_1
 * @see MPU60X0_PWR1_TEMP_DIS_BIT
 */
void MPU60X0::setTempSensorEnabled(bool enabled) {
    // 1 is actually disabled here
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_1, MPU60X0_PWR1_TEMP_DIS_BIT, !enabled);
}
/** Get clock source setting.
 * @return Current clock source setting
 * @see MPU60X0_RA_PWR_MGMT_1
 * @see MPU60X0_PWR1_CLKSEL_BIT
 * @see MPU60X0_PWR1_CLKSEL_LENGTH
 */
uint8_t MPU60X0::getClockSource() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_1, MPU60X0_PWR1_CLKSEL_BIT, MPU60X0_PWR1_CLKSEL_LENGTH, buffer);
    return buffer[0];
}
/** Set clock source setting.
 * An internal 8MHz oscillator, gyroscope based clock, or external sources can
 * be selected as the MPU-60X0 clock source. When the internal 8 MHz oscillator
 * or an external source is chosen as the clock source, the MPU-60X0 can operate
 * in low power modes with the gyroscopes disabled.
 *
 * Upon power up, the MPU-60X0 clock source defaults to the internal oscillator.
 * However, it is highly recommended that the device be configured to use one of
 * the gyroscopes (or an external clock source) as the clock reference for
 * improved stability. The clock source can be selected according to the following table:
 *
 * <pre>
 * CLK_SEL | Clock Source
 * --------+--------------------------------------
 * 0       | Internal oscillator
 * 1       | PLL with X Gyro reference
 * 2       | PLL with Y Gyro reference
 * 3       | PLL with Z Gyro reference
 * 4       | PLL with external 32.768kHz reference
 * 5       | PLL with external 19.2MHz reference
 * 6       | Reserved
 * 7       | Stops the clock and keeps the timing generator in reset
 * </pre>
 *
 * @param source New clock source setting
 * @see getClockSource()
 * @see MPU60X0_RA_PWR_MGMT_1
 * @see MPU60X0_PWR1_CLKSEL_BIT
 * @see MPU60X0_PWR1_CLKSEL_LENGTH
 */
void MPU60X0::setClockSource(uint8_t source) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_1, MPU60X0_PWR1_CLKSEL_BIT, MPU60X0_PWR1_CLKSEL_LENGTH, source);
}

// PWR_MGMT_2 register

/** Get wake frequency in Accel-Only Low Power Mode.
 * The MPU-60X0 can be put into Accerlerometer Only Low Power Mode by setting
 * PWRSEL to 1 in the Power Management 1 register (Register 107). In this mode,
 * the device will power off all devices except for the primary I2C interface,
 * waking only the accelerometer at fixed intervals to take a single
 * measurement. The frequency of wake-ups can be configured with LP_WAKE_CTRL
 * as shown below:
 *
 * <pre>
 * LP_WAKE_CTRL | Wake-up Frequency
 * -------------+------------------
 * 0            | 1.25 Hz
 * 1            | 2.5 Hz
 * 2            | 5 Hz
 * 3            | 10 Hz
 * <pre>
 *
 * For further information regarding the MPU-60X0's power modes, please refer to
 * Register 107.
 *
 * @return Current wake frequency
 * @see MPU60X0_RA_PWR_MGMT_2
 */
uint8_t MPU60X0::getWakeFrequency() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_LP_WAKE_CTRL_BIT, MPU60X0_PWR2_LP_WAKE_CTRL_LENGTH, buffer);
    return buffer[0];
}
/** Set wake frequency in Accel-Only Low Power Mode.
 * @param frequency New wake frequency
 * @see MPU60X0_RA_PWR_MGMT_2
 */
void MPU60X0::setWakeFrequency(uint8_t frequency) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_LP_WAKE_CTRL_BIT, MPU60X0_PWR2_LP_WAKE_CTRL_LENGTH, frequency);
}

/** Get X-axis accelerometer standby enabled status.
 * If enabled, the X-axis will not gather or report data (or use power).
 * @return Current X-axis standby enabled status
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_XA_BIT
 */
bool MPU60X0::getStandbyXAccelEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_XA_BIT, buffer);
    return buffer[0];
}
/** Set X-axis accelerometer standby enabled status.
 * @param New X-axis standby enabled status
 * @see getStandbyXAccelEnabled()
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_XA_BIT
 */
void MPU60X0::setStandbyXAccelEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_XA_BIT, enabled);
}
/** Get Y-axis accelerometer standby enabled status.
 * If enabled, the Y-axis will not gather or report data (or use power).
 * @return Current Y-axis standby enabled status
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_YA_BIT
 */
bool MPU60X0::getStandbyYAccelEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_YA_BIT, buffer);
    return buffer[0];
}
/** Set Y-axis accelerometer standby enabled status.
 * @param New Y-axis standby enabled status
 * @see getStandbyYAccelEnabled()
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_YA_BIT
 */
void MPU60X0::setStandbyYAccelEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_YA_BIT, enabled);
}
/** Get Z-axis accelerometer standby enabled status.
 * If enabled, the Z-axis will not gather or report data (or use power).
 * @return Current Z-axis standby enabled status
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_ZA_BIT
 */
bool MPU60X0::getStandbyZAccelEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_ZA_BIT, buffer);
    return buffer[0];
}
/** Set Z-axis accelerometer standby enabled status.
 * @param New Z-axis standby enabled status
 * @see getStandbyZAccelEnabled()
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_ZA_BIT
 */
void MPU60X0::setStandbyZAccelEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_ZA_BIT, enabled);
}
/** Get X-axis gyroscope standby enabled status.
 * If enabled, the X-axis will not gather or report data (or use power).
 * @return Current X-axis standby enabled status
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_XG_BIT
 */
bool MPU60X0::getStandbyXGyroEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_XG_BIT, buffer);
    return buffer[0];
}
/** Set X-axis gyroscope standby enabled status.
 * @param New X-axis standby enabled status
 * @see getStandbyXGyroEnabled()
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_XG_BIT
 */
void MPU60X0::setStandbyXGyroEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_XG_BIT, enabled);
}
/** Get Y-axis gyroscope standby enabled status.
 * If enabled, the Y-axis will not gather or report data (or use power).
 * @return Current Y-axis standby enabled status
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_YG_BIT
 */
bool MPU60X0::getStandbyYGyroEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_YG_BIT, buffer);
    return buffer[0];
}
/** Set Y-axis gyroscope standby enabled status.
 * @param New Y-axis standby enabled status
 * @see getStandbyYGyroEnabled()
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_YG_BIT
 */
void MPU60X0::setStandbyYGyroEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_YG_BIT, enabled);
}
/** Get Z-axis gyroscope standby enabled status.
 * If enabled, the Z-axis will not gather or report data (or use power).
 * @return Current Z-axis standby enabled status
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_ZG_BIT
 */
bool MPU60X0::getStandbyZGyroEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_ZG_BIT, buffer);
    return buffer[0];
}
/** Set Z-axis gyroscope standby enabled status.
 * @param New Z-axis standby enabled status
 * @see getStandbyZGyroEnabled()
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see MPU60X0_PWR2_STBY_ZG_BIT
 */
void MPU60X0::setStandbyZGyroEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, MPU60X0_PWR2_STBY_ZG_BIT, enabled);
}

/** Disable MPU-6050 standby mode. Really only used for MPU-9250
 * @param 
 * @see 
 * @see MPU60X0_RA_PWR_MGMT_2
 * @see 
 */
void MPU60X0::setStandbyDisable() {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_PWR_MGMT_2, 0x00);
}

// FIFO_COUNT* registers

/** Get current FIFO buffer size.
 * This value indicates the number of bytes stored in the FIFO buffer. This
 * number is in turn the number of bytes that can be read from the FIFO buffer
 * and it is directly proportional to the number of samples available given the
 * set of sensor data bound to be stored in the FIFO (register 35 and 36).
 * @return Current FIFO buffer size
 */
uint16_t MPU60X0::getFIFOCount() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_FIFO_COUNTH, 2, buffer);
    return (((uint16_t)buffer[0]) << 8) | buffer[1];
}

// FIFO_R_W register

/** Get byte from FIFO buffer.
 * This register is used to read and write data from the FIFO buffer. Data is
 * written to the FIFO in order of register number (from lowest to highest). If
 * all the FIFO enable flags (see below) are enabled and all External Sensor
 * Data registers (Registers 73 to 96) are associated with a Slave device, the
 * contents of registers 59 through 96 will be written in order at the Sample
 * Rate.
 *
 * The contents of the sensor data registers (Registers 59 to 96) are written
 * into the FIFO buffer when their corresponding FIFO enable flags are set to 1
 * in FIFO_EN (Register 35). An additional flag for the sensor data registers
 * associated with I2C Slave 3 can be found in I2C_MST_CTRL (Register 36).
 *
 * If the FIFO buffer has overflowed, the status bit FIFO_OFLOW_INT is
 * automatically set to 1. This bit is located in INT_STATUS (Register 58).
 * When the FIFO buffer has overflowed, the oldest data will be lost and new
 * data will be written to the FIFO.
 *
 * If the FIFO buffer is empty, reading this register will return the last byte
 * that was previously read from the FIFO until new data is available. The user
 * should check FIFO_COUNT to ensure that the FIFO buffer is not read when
 * empty.
 *
 * @return Byte from FIFO buffer
 */
uint8_t MPU60X0::getFIFOByte() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_FIFO_R_W, buffer);
    return buffer[0];
}

void MPU60X0::getFIFOBytes(uint8_t *data, uint8_t length) {
    I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_FIFO_R_W, length, data);
}

/** Write byte to FIFO buffer.
 * @see getFIFOByte()
 * @see MPU60X0_RA_FIFO_R_W
 */
void MPU60X0::setFIFOByte(uint8_t data) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_FIFO_R_W, data);
}

// WHO_AM_I register

/** Get Device ID.
 * This register is used to verify the identity of the device (0b110100).
 * @return Device ID (should be 0x68, 104 dec, 150 oct)
 * @see MPU60X0_RA_WHO_AM_I
 * @see MPU60X0_WHO_AM_I_BIT
 * @see MPU60X0_WHO_AM_I_LENGTH
 */
uint8_t MPU60X0::getDeviceID() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_WHO_AM_I, MPU60X0_WHO_AM_I_BIT, MPU60X0_WHO_AM_I_LENGTH, buffer);
    return buffer[0];
}
/** Set Device ID.
 * Write a new ID into the WHO_AM_I register (no idea why this should ever be
 * necessary though).
 * @param id New device ID to set.
 * @see getDeviceID()
 * @see MPU60X0_RA_WHO_AM_I
 * @see MPU60X0_WHO_AM_I_BIT
 * @see MPU60X0_WHO_AM_I_LENGTH
 */
void MPU60X0::setDeviceID(uint8_t id) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_WHO_AM_I, MPU60X0_WHO_AM_I_BIT, MPU60X0_WHO_AM_I_LENGTH, id);
}

// ======== UNDOCUMENTED/DMP REGISTERS/METHODS ========

uint8_t MPU60X0::getOTPBankValid() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_XG_OFFS_TC, MPU60X0_TC_OTP_BNK_VLD_BIT, buffer);
    return buffer[0];
}
void MPU60X0::setOTPBankValid(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_XG_OFFS_TC, MPU60X0_TC_OTP_BNK_VLD_BIT, enabled);
}

// XG_OFFS_TC register

int8_t MPU60X0::getXGyroOffset() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_XG_OFFS_TC, MPU60X0_TC_OFFSET_BIT, MPU60X0_TC_OFFSET_LENGTH, buffer);
    return buffer[0];
}
void MPU60X0::setXGyroOffset(int8_t offset) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_XG_OFFS_TC, MPU60X0_TC_OFFSET_BIT, MPU60X0_TC_OFFSET_LENGTH, offset);
}

// YG_OFFS_TC register

int8_t MPU60X0::getYGyroOffset() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_YG_OFFS_TC, MPU60X0_TC_OFFSET_BIT, MPU60X0_TC_OFFSET_LENGTH, buffer);
    return buffer[0];
}
void MPU60X0::setYGyroOffset(int8_t offset) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_YG_OFFS_TC, MPU60X0_TC_OFFSET_BIT, MPU60X0_TC_OFFSET_LENGTH, offset);
}

// ZG_OFFS_TC register

int8_t MPU60X0::getZGyroOffset() {
    I2Cdev::readBits(bSPI, devAddr, MPU60X0_RA_ZG_OFFS_TC, MPU60X0_TC_OFFSET_BIT, MPU60X0_TC_OFFSET_LENGTH, buffer);
    return buffer[0];
}
void MPU60X0::setZGyroOffset(int8_t offset) {
    I2Cdev::writeBits(bSPI, devAddr, MPU60X0_RA_ZG_OFFS_TC, MPU60X0_TC_OFFSET_BIT, MPU60X0_TC_OFFSET_LENGTH, offset);
}

// X_FINE_GAIN register

int8_t MPU60X0::getXFineGain() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_X_FINE_GAIN, buffer);
    return buffer[0];
}
void MPU60X0::setXFineGain(int8_t gain) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_X_FINE_GAIN, gain);
}

// Y_FINE_GAIN register

int8_t MPU60X0::getYFineGain() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_Y_FINE_GAIN, buffer);
    return buffer[0];
}
void MPU60X0::setYFineGain(int8_t gain) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_Y_FINE_GAIN, gain);
}

// Z_FINE_GAIN register

int8_t MPU60X0::getZFineGain() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_Z_FINE_GAIN, buffer);
    return buffer[0];
}
void MPU60X0::setZFineGain(int8_t gain) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_Z_FINE_GAIN, gain);
}

// XA_OFFS_* registers

int16_t MPU60X0::getXAccelOffset() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_XA_OFFS_H, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
void MPU60X0::setXAccelOffset(int16_t offset) {
   I2Cdev::writeWord(bSPI, devAddr, MPU60X0_RA_XA_OFFS_H, offset);
}

// YA_OFFS_* register

int16_t MPU60X0::getYAccelOffset() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_YA_OFFS_H, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
void MPU60X0::setYAccelOffset(int16_t offset) {
   I2Cdev::writeWord(bSPI, devAddr, MPU60X0_RA_YA_OFFS_H, offset);
}

// ZA_OFFS_* register

int16_t MPU60X0::getZAccelOffset() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_ZA_OFFS_H, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
void MPU60X0::setZAccelOffset(int16_t offset) {
   I2Cdev::writeWord(bSPI, devAddr, MPU60X0_RA_ZA_OFFS_H, offset);
}

// XG_OFFS_USR* registers

int16_t MPU60X0::getXGyroOffsetUser() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_XG_OFFS_USRH, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
void MPU60X0::setXGyroOffsetUser(int16_t offset) {
   I2Cdev::writeWord(bSPI, devAddr, MPU60X0_RA_XG_OFFS_USRH, offset);
}

// YG_OFFS_USR* register

int16_t MPU60X0::getYGyroOffsetUser() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_YG_OFFS_USRH, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
void MPU60X0::setYGyroOffsetUser(int16_t offset) {
   I2Cdev::writeWord(bSPI, devAddr, MPU60X0_RA_YG_OFFS_USRH, offset);
}

// ZG_OFFS_USR* register

int16_t MPU60X0::getZGyroOffsetUser() {
   I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_ZG_OFFS_USRH, 2, buffer);
    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
void MPU60X0::setZGyroOffsetUser(int16_t offset) {
   I2Cdev::writeWord(bSPI, devAddr, MPU60X0_RA_ZG_OFFS_USRH, offset);
}

#if defined(__AVR__)
// INT_ENABLE register (DMP functions)

bool MPU60X0::getIntPLLReadyEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_PLL_RDY_INT_BIT, buffer);
    return buffer[0];
}
void MPU60X0::setIntPLLReadyEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_PLL_RDY_INT_BIT, enabled);
}
bool MPU60X0::getIntDMPEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_DMP_INT_BIT, buffer);
    return buffer[0];
}
void MPU60X0::setIntDMPEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, MPU60X0_INTERRUPT_DMP_INT_BIT, enabled);
}

// DMP_INT_STATUS

bool MPU60X0::getDMPInt5Status() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_DMP_INT_STATUS, MPU60X0_DMPINT_5_BIT, buffer);
    return buffer[0];
}
bool MPU60X0::getDMPInt4Status() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_DMP_INT_STATUS, MPU60X0_DMPINT_4_BIT, buffer);
    return buffer[0];
}
bool MPU60X0::getDMPInt3Status() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_DMP_INT_STATUS, MPU60X0_DMPINT_3_BIT, buffer);
    return buffer[0];
}
bool MPU60X0::getDMPInt2Status() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_DMP_INT_STATUS, MPU60X0_DMPINT_2_BIT, buffer);
    return buffer[0];
}
bool MPU60X0::getDMPInt1Status() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_DMP_INT_STATUS, MPU60X0_DMPINT_1_BIT, buffer);
    return buffer[0];
}
bool MPU60X0::getDMPInt0Status() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_DMP_INT_STATUS, MPU60X0_DMPINT_0_BIT, buffer);
    return buffer[0];
}

// INT_STATUS register (DMP functions)

bool MPU60X0::getIntPLLReadyStatus() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_STATUS, MPU60X0_INTERRUPT_PLL_RDY_INT_BIT, buffer);
    return buffer[0];
}
bool MPU60X0::getIntDMPStatus() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_INT_STATUS, MPU60X0_INTERRUPT_DMP_INT_BIT, buffer);
    return buffer[0];
}

// USER_CTRL register (DMP functions)

bool MPU60X0::getDMPEnabled() {
    I2Cdev::readBit(bSPI, devAddr, MPU60X0_RA_USER_CTRL, MPU60X0_USERCTRL_DMP_EN_BIT, buffer);
    return buffer[0];
}
void MPU60X0::setDMPEnabled(bool enabled) {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_USER_CTRL, MPU60X0_USERCTRL_DMP_EN_BIT, enabled);
}
void MPU60X0::resetDMP() {
    I2Cdev::writeBit(bSPI, devAddr, MPU60X0_RA_USER_CTRL, MPU60X0_USERCTRL_DMP_RESET_BIT, true);
}

// BANK_SEL register

void MPU60X0::setMemoryBank(uint8_t bank, bool prefetchEnabled, bool userBank) {
    bank &= 0x1F;
    if (userBank) bank |= 0x20;
    if (prefetchEnabled) bank |= 0x40;
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_BANK_SEL, bank);
}

// MEM_START_ADDR register

void MPU60X0::setMemoryStartAddress(uint8_t address) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_MEM_START_ADDR, address);
}

// MEM_R_W register

uint8_t MPU60X0::readMemoryByte() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_MEM_R_W, buffer);
    return buffer[0];
}
void MPU60X0::writeMemoryByte(uint8_t data) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_MEM_R_W, data);
}
void MPU60X0::readMemoryBlock(uint8_t *data, uint16_t dataSize, uint8_t bank, uint8_t address) {
    setMemoryBank(bank);
    setMemoryStartAddress(address);
    uint8_t chunkSize;
    for (uint16_t i = 0; i < dataSize;) {
        // determine correct chunk size according to bank position and data size
        chunkSize = MPU60X0_DMP_MEMORY_CHUNK_SIZE;

        // make sure we don't go past the data size
        if (i + chunkSize > dataSize) chunkSize = dataSize - i;

        // make sure this chunk doesn't go past the bank boundary (256 bytes)
        if (chunkSize > 256 - address) chunkSize = 256 - address;

        // read the chunk of data as specified
       I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_MEM_R_W, chunkSize, data + i);
        
        // increase byte index by [chunkSize]
        i += chunkSize;

        // uint8_t automatically wraps to 0 at 256
        address + chunkSize;

        // if we aren't done, update bank (if necessary) and address
        if (i < dataSize) {
            if (address == 0) bank++;
            setMemoryBank(bank);
            setMemoryStartAddress(address);
        }
    }
}
bool MPU60X0::writeMemoryBlock(const uint8_t *data, uint16_t dataSize, uint8_t bank, uint8_t address, bool verify, bool useProgMem) {
    setMemoryBank(bank);
    setMemoryStartAddress(address);
    uint8_t chunkSize;
    uint8_t *verifyBuffer;
    uint8_t *progBuffer;
    uint16_t i;
    uint8_t j;
    if (verify) verifyBuffer = (uint8_t *)malloc(MPU60X0_DMP_MEMORY_CHUNK_SIZE);
    if (useProgMem) progBuffer = (uint8_t *)malloc(MPU60X0_DMP_MEMORY_CHUNK_SIZE);
    for (i = 0; i < dataSize;) {
        // determine correct chunk size according to bank position and data size
        chunkSize = MPU60X0_DMP_MEMORY_CHUNK_SIZE;

        // make sure we don't go past the data size
        if (i + chunkSize > dataSize) chunkSize = dataSize - i;

        // make sure this chunk doesn't go past the bank boundary (256 bytes)
        if (chunkSize > 256 - address) chunkSize = 256 - address;
        
        if (useProgMem) {
            // write the chunk of data as specified
            for (j = 0; j < chunkSize; j++) progBuffer[j] = pgm_read_byte(data + i + j);
        } else {
            // write the chunk of data as specified
            progBuffer = (uint8_t *)data + i;
        }

        I2Cdev::writeBytes(bSPI, devAddr, MPU60X0_RA_MEM_R_W, chunkSize, progBuffer);

        // verify data if needed
        if (verify && verifyBuffer) {
            setMemoryBank(bank);
            setMemoryStartAddress(address);
            I2Cdev::readBytes(bSPI, devAddr, MPU60X0_RA_MEM_R_W, chunkSize, verifyBuffer);
            if (memcmp(progBuffer, verifyBuffer, chunkSize) != 0) {
                /*Serial.print("Block write verification error, bank ");
                Serial.print(bank, DEC);
                Serial.print(", address ");
                Serial.print(address, DEC);
                Serial.print("!\nExpected:");
                for (j = 0; j < chunkSize; j++) {
                    Serial.print(" 0x");
                    if (progBuffer[j] < 16) Serial.print("0");
                    Serial.print(progBuffer[j], HEX);
                }
                Serial.print("\nReceived:");
                for (uint8_t j = 0; j < chunkSize; j++) {
                    Serial.print(" 0x");
                    if (verifyBuffer[i + j] < 16) Serial.print("0");
                    Serial.print(verifyBuffer[i + j], HEX);
                }
                Serial.print("\n");*/
                free(verifyBuffer);
                if (useProgMem) free(progBuffer);
                return false; // uh oh.
            }
        }

        // increase byte index by [chunkSize]
        i += chunkSize;

        // uint8_t automatically wraps to 0 at 256
        address += chunkSize;

        // if we aren't done, update bank (if necessary) and address
        if (i < dataSize) {
            if (address == 0) bank++;
            setMemoryBank(bank);
            setMemoryStartAddress(address);
        }
    }
    if (verify) free(verifyBuffer);
    if (useProgMem) free(progBuffer);
    return true;
}
bool MPU60X0::writeProgMemoryBlock(const uint8_t *data, uint16_t dataSize, uint8_t bank, uint8_t address, bool verify) {
    return writeMemoryBlock(data, dataSize, bank, address, verify, true);
}
bool MPU60X0::writeDMPConfigurationSet(const uint8_t *data, uint16_t dataSize, bool useProgMem) {
    uint8_t *progBuffer, success, special;
    uint16_t i, j;
    if (useProgMem) {
        progBuffer = (uint8_t *)malloc(8); // assume 8-byte blocks, realloc later if necessary
    }

    // config set data is a long string of blocks with the following structure:
    // [bank] [offset] [length] [byte[0], byte[1], ..., byte[length]]
    uint8_t bank, offset, length;
    for (i = 0; i < dataSize;) {
        if (useProgMem) {
            bank = pgm_read_byte(data + i++);
            offset = pgm_read_byte(data + i++);
            length = pgm_read_byte(data + i++);
        } else {
            bank = data[i++];
            offset = data[i++];
            length = data[i++];
        }

        // write data or perform special action
        if (length > 0) {
            // regular block of data to write
            /*Serial.print("Writing config block to bank ");
            Serial.print(bank);
            Serial.print(", offset ");
            Serial.print(offset);
            Serial.print(", length=");
            Serial.println(length);*/
            if (useProgMem) {
                if (sizeof(progBuffer) < length) progBuffer = (uint8_t *)realloc(progBuffer, length);
                for (j = 0; j < length; j++) progBuffer[j] = pgm_read_byte(data + i + j);
            } else {
                progBuffer = (uint8_t *)data + i;
            }
            success = writeMemoryBlock(progBuffer, length, bank, offset, true);
            i += length;
        } else {
            // special instruction
            // NOTE: this kind of behavior (what and when to do certain things)
            // is totally undocumented. This code is in here based on observed
            // behavior only, and exactly why (or even whether) it has to be here
            // is anybody's guess for now.
            if (useProgMem) {
                special = pgm_read_byte(data + i++);
            } else {
                special = data[i++];
            }
            /*Serial.print("Special command code ");
            Serial.print(special, HEX);
            Serial.println(" found...");*/
            if (special == 0x01) {
                // enable DMP-related interrupts
                
                //setIntZeroMotionEnabled(true);
                //setIntFIFOBufferOverflowEnabled(true);
                //setIntDMPEnabled(true);
                I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_INT_ENABLE, 0x32);  // single operation

                success = true;
            } else {
                // unknown special command
                success = false;
            }
        }
        
        if (!success) {
            if (useProgMem) free(progBuffer);
            return false; // uh oh
        }
    }
    if (useProgMem) free(progBuffer);
    return true;
}
bool MPU60X0::writeProgDMPConfigurationSet(const uint8_t *data, uint16_t dataSize) {
    return writeDMPConfigurationSet(data, dataSize, true);
}



// DMP_CFG_1 register

uint8_t MPU60X0::getDMPConfig1() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_DMP_CFG_1, buffer);
    return buffer[0];
}
void MPU60X0::setDMPConfig1(uint8_t config) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_DMP_CFG_1, config);
}

// DMP_CFG_2 register

uint8_t MPU60X0::getDMPConfig2() {
    I2Cdev::readByte(bSPI, devAddr, MPU60X0_RA_DMP_CFG_2, buffer);
    return buffer[0];
}
void MPU60X0::setDMPConfig2(uint8_t config) {
    I2Cdev::writeByte(bSPI, devAddr, MPU60X0_RA_DMP_CFG_2, config);
}

#endif
