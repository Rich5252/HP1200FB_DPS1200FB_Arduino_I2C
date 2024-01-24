//--------------------------------------------------------------------------
//Derived from: https://github.com/raplin/DPS-1200FB
// Lots of other useful info here: https://github.com/slundell/dps_charger
//--------------------------------------------------------------------------


//SoftI2C-master lib MUST BE THIS ONE:
//https://github.com/yasir-shahzad/SoftI2C
//

#include <SoftI2C.h>

//initialise I2C
// Nano pins are A4 - sda and A5 - scl
// NOTE PSU pin 30 - Gnd, 31 - SDA, 32 - SCL
// Also requires pullups enabled on Nano (external pullups therefore not required)

SoftI2C I2C = SoftI2C(A4, A5, true);            //sda, scl, pullup

void setup()
{
  Serial.begin(9600);
  Serial.println("\nHP1200FB_basic");

  // init IC2 interface
  I2C.begin();
}

void loop()
{
  //see list of registers below
  // eg 0x07:["OUTPUT_VOLTAGE",254.5]
  
  int reg = 0x07;
  float scale = 254.5;
  String strName = "OUTPUT_VOLTAGE";
  String strUnits = "Volts";

  float val = readPSU(reg, scale);        //bit shift on register number is done in readPSU

  Serial.print(strName + ": ");
  Serial.print(val);
  Serial.println(" " + strUnits);
  delay(500);
}


// basic function to read a single 16 bit register from PSU
float readPSU(uint8_t reg, float scale)
{
  // i2c address is made up of base and slot value. Slot is 3 bits on PSU pins 27 (lsb). 28 and 29 (msb
  // if all left open circuit slot is 7
  uint8_t slot = 7;
  uint8_t i2cAddress = 0x58 + slot;

  //calculate essential checksum that is part of data request instruction
  uint8_t cs = (reg << 1) + (i2cAddress << 1);
  uint8_t regCS = ((0xff-cs)+1)&0xff;             

  //Basic sequence is to write out the data request instruction
  //  followed by reading the two byte reply.
  //write [address,register,checksum] and then read two bytes (send address+read bit, read lsb,read msb)
  // These can be done in seperate "transmission" sections
  
  // Send request to PSU
  I2C.beginTransmission(i2cAddress);   //enables Nano as controller and send/checks address OK
  I2C.write(reg << 1);                 //register to read
  I2C.write(regCS);                    //essential checksum otherwise not accepted.
  uint8_t ret = I2C.endTransmission();

  String str = "i2cAddress accepted";
  if (ret) {str = "i2cAddress not acknowledged error";}
  Serial.println(str);

  //read of the register
  //first request read from PSU i2cAddress (it was primed with the instruction written above)
  // the "requestFrom" function reads into library's local buffer
  // return value "n" is number of successfully read bytes
  uint8_t n = I2C.requestFrom(i2cAddress, (uint8_t) 2);  //get 2 bytes from i2c device
  int lsb = I2C.read();                                  //then read the pair of bytes from buffer
  int msb = I2C.read();
  int val = msb * 256 + lsb;
  
  Serial.print("reg 0x00 = ");
  Serial.print(reg, HEX);
  Serial.print(", ");
  Serial.print(n);                                      //if "n" is not 2 then something went wrong
  Serial.println(" bytes read");
  
  return (float) val / scale;
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
// register info from original python source
//      https://github.com/raplin/DPS-1200FB
//---------------------------------------------------------------------------------------
// HEX register value needs a shift left applied when sent to I2C and used to calc checksum
// HEX like "0x50>>1" means use 0x50 shifted right as input to readPSU function
//---------------------------------------------------------------------------------------
/*
    #Readable registers - some of these are slightly guessed - comments welcome if you figure something new out or have a correction.
    REGS={
        #note when looking at PIC disasm table; "lookup_ram_to_read_for_cmd", below numbers are <<1
        #the second arg is the scale factor
        0x01:["FLAGS",0],           #not sure but includes e.g. "power good"
        0x04:["INPUT_VOLTAGE",32.0], #e.g. 120 (volts)
        0x05:["AMPS_IN",128.0],
        0x06:["WATTS_IN",2.0],
        0x07:["OUTPUT_VOLTAGE",254.5], #pretty sure this is right; unclear why scale is /254.5 not /256 but it's wrong - can't see how they'd not be measuring this to high precision
        0x08:["AMPS_OUT",128.0],  #rather inaccurate at low output <10A (reads under) - appears to have internal load for stability so always reads about 1.5 even open circuit
        0x09:["WATTS_OUT",2.0],
        0x0d:["TEMP1_INTAKE_FARENHEIT",32.0],   # this is a guess - may be C or F but F looks more right
        0x0e:["TEMP2_INTERNAL_FARENHEIT",32.0], 
        0x0f:["FAN_SPEED_RPM",1], #total guess at scale but this is def fan speed it may be counting ticks from the fan sensor which seem to be typically 2 ticks/revolution
        0x1a:["?flags",0],                      #unknown (from disassembly)
        0x1b:["?voltage",1],                    #unknown (from disassembly)
        (0x2c>>1):["WATT_SECONDS_IN",-4.0], #this is a special case; uses two consecutive regs to make a 32-bit value (the minus scale factor is a flag for that)
        (0x30>>1):["ON_SECONDS",2.0],
        (0x32>>1):["PEAK_WATTS_IN",2.0],
        (0x34>>1):["MIN_AMPS_IN",128.0],
        (0x36>>1):["PEAK_AMPS_OUT",128.0],
        (0x3A>>1):["COOL_FLAGS1",0],             #unknown (from disassembly)
        (0x3c>>1):["COOL_FLAGS2",0],             #unknown (from disassembly)
        (0x40>>1):["FAN_TARGET_RPM",1],          #unknown (from disassembly)
        (0x44>>1):["VOLTAGE_THRESHOLD_1",1],    #unknown (from disassembly)
        (0x46>>1):["VOLTAGE_THRESHOLD_2",1],    #unknown (from disassembly)
        (0x50>>1):["MAYBE_UNDERVOLTAGE_THRESH",32.0],    #unknown (from disassembly)
        (0x52>>1):["MAYBE_OVERVOLTAGE_THRESH",32.0],    #unknown (from disassembly)
        #reading 0x57 reads internal EEPROM space in CPU (just logging info, e.g. hours in use)
        }
 */
