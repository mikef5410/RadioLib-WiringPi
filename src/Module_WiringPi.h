#if !defined(_RADIOLIB_MODULE_H)
#define _RADIOLIB_MODULE_H
#include <stdint.h>
#include "wiringPi.h"
#include "wiringPiSPI.h"
//#include "spi.h" //linux_device_access



#include "TypeDef.h"
#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3
#ifndef LSBFIRST
#define LSBFIRST 0
#endif
#ifndef MSBFIRST
#define MSBFIRST 1
#endif


// Arduino look-alike layer over WiringPi SPI
class SPISettings {
public:
  //SPI_HANDLE handle;
  uint8_t channel;
  uint32_t clock; //clock frequency in Hz
  uint8_t bitOrder; //MSBFIRST or LSBFIRST
  uint8_t dataMode; //0,1,2,3
  bool IPcontrolsCE;
  int gpioCE;
  
  SPISettings(uint32_t _clock, uint8_t _bitOrder, uint8_t _dataMode):
    clock(_clock),
    bitOrder(_bitOrder),
    dataMode(_dataMode)
  {
    IPcontrolsCE=1;
    channel=0;
  }

  void setChannel(uint8_t chan) {
    channel=chan; //Set the chip enable line 0 -> CE0, 1-> CE1
  }

  void setGPIOCE(int gpio) { //GPIO#gpio is the CE. Code needs to manually operate it in begin/endTransaction
    gpioCE=gpio;
    IPcontrolsCE=0;
    channel=0;
    pinMode(gpio,OUTPUT);
    digitalWrite(gpio,HIGH);
  }

  friend class SPIClass;
};

class SPIClass {
public:
  SPISettings& settings;
  int SPIfd=-1;
  void(*preTransHook)() = NULL; //Use these hooks if chip enable is a regular gpio
  void(*postTransHook)() = NULL;
  // Initialize the SPI library

  SPIClass(SPISettings& _settings):
    settings(_settings)
  {
  }
  
  void begin() {
  }

  void beginTransaction(SPISettings& _settings) {
    settings=_settings;
    //settings.handle=SpiOpenPort(settings.channel, 8, settings.clock, settings.dataMode, 0);
    if (SPIfd<1) { //Only need to setup once
      SPIfd=wiringPiSPISetupMode(settings.channel, settings.clock, settings.dataMode);
    }
    if (preTransHook != NULL) (*preTransHook)();
  }
  uint8_t transfer(uint8_t data) {
    wiringPiSPIDataRW(settings.channel, (unsigned char *)&data, 1);
    //SpiWriteAndRead(settings.handle, &data, &data, 1, 0);
    return(data);
  }
  uint16_t transfer16(uint16_t data) { //not used
    wiringPiSPIDataRW(settings.channel, (unsigned char *)&data, 2);
    //SpiWriteAndRead(settings.handle, &data, &data, 2, 0);
    return(data);
  }
  void transfer(void *buf, size_t count) {
    //SpiWriteAndRead(settings.handle, (uint8_t *)buf, (uint8_t *)buf, count, 0);
    wiringPiSPIDataRW(settings.channel, (unsigned char *)buf, count);
  }
  void endTransaction(void) {
    if (postTransHook != NULL) (*postTransHook)();
    //SpiClosePort(settings.handle);
  }
  void end() {
  }
  void setBitOrder(uint8_t bitOrder) {
  }
  void setDataMode(uint8_t dataMode) {
  }
  void setClockDivider(uint8_t clockDiv) {
  }
  void attachInterrupt() { };
  void detachInterrupt() { };
};


/*!
  \class Module

  \brief Implements all common low-level methods to control the wireless module.
  Every module class contains one private instance of this class.
*/
class Module {
public:
  /*!
    \brief Default constructor.

    \param cs Pin to be used as chip select.

    \param irq Pin to be used as interrupt/GPIO.

    \param rst Pin to be used as hardware reset for the module.

    \param gpio Pin to be used as additional interrupt/GPIO.
  */
  Module(RADIOLIB_PIN_TYPE cs, RADIOLIB_PIN_TYPE irq, RADIOLIB_PIN_TYPE rst, RADIOLIB_PIN_TYPE gpio = RADIOLIB_NC);


  /*!
    \brief Copy constructor.

    \param mod Module instance to copy.
  */
  Module(const Module& mod);

  /*!
    \brief Overload for assignment operator.

    \param frame rvalue Module.
  */
  Module& operator=(const Module& mod);

  // public member variables

  /*!
    \brief Basic SPI read command. Defaults to 0x00.
  */
  uint8_t SPIreadCommand = 0b00000000;

  /*!
    \brief Basic SPI write command. Defaults to 0x80.
  */
  uint8_t SPIwriteCommand = 0b10000000;

  // basic methods

  /*!
    \brief Initialize low-level module control.
  */
  void init();

  /*!
    \brief Terminate low-level module control.
  */
  void term();

  // SPI methods

  /*!
    \brief SPI read method that automatically masks unused bits. This method is the preferred SPI read mechanism.

    \param reg Address of SPI register to read.

    \param msb Most significant bit of the register variable. Bits above this one will be masked out.

    \param lsb Least significant bit of the register variable. Bits below this one will be masked out.

    \returns Masked register value or status code.
  */
  int16_t SPIgetRegValue(uint8_t reg, uint8_t msb = 7, uint8_t lsb = 0);

  /*!
    \brief Overwrite-safe SPI write method with verification. This method is the preferred SPI write mechanism.

    \param reg Address of SPI register to write.

    \param value Single byte value that will be written to the SPI register.

    \param msb Most significant bit of the register variable. Bits above this one will not be affected by the write operation.

    \param lsb Least significant bit of the register variable. Bits below this one will not be affected by the write operation.

    \param checkInterval Number of milliseconds between register writing and verification reading. Some registers need up to 10ms to process the change.

    \param checkMask Mask of bits to check, only bits set to 1 will be verified.

    \returns \ref status_codes
  */
  int16_t SPIsetRegValue(uint8_t reg, uint8_t value, uint8_t msb = 7, uint8_t lsb = 0, uint8_t checkInterval = 2, uint8_t checkMask = 0xFF);

  /*!
    \brief SPI burst read method.

    \param reg Address of SPI register to read.

    \param numBytes Number of bytes that will be read.

    \param inBytes Pointer to array that will hold the read data.
  */
  void SPIreadRegisterBurst(uint8_t reg, uint8_t numBytes, uint8_t* inBytes);

  /*!
    \brief SPI basic read method. Use of this method is reserved for special cases, SPIgetRegValue should be used instead.

    \param reg Address of SPI register to read.

    \returns Value that was read from register.
  */
  uint8_t SPIreadRegister(uint8_t reg);

  /*!
    \brief SPI burst write method.

    \param reg Address of SPI register to write.

    \param data Pointer to array that holds the data that will be written.

    \param numBytes Number of bytes that will be written.
  */
  void SPIwriteRegisterBurst(uint8_t reg, uint8_t* data, uint8_t numBytes);

  /*!
    \brief SPI basic write method. Use of this method is reserved for special cases, SPIsetRegValue should be used instead.

    \param reg Address of SPI register to write.

    \param data Value that will be written to the register.
  */
  void SPIwriteRegister(uint8_t reg, uint8_t data);

  /*!
    \brief SPI single transfer method.

    \param cmd SPI access command (read/write/burst/...).

    \param reg Address of SPI register to transfer to/from.

    \param dataOut Data that will be transfered from master to slave.

    \param dataIn Data that was transfered from slave to master.

    \param numBytes Number of bytes to transfer.
  */
  void SPItransfer(uint8_t cmd, uint8_t reg, uint8_t* dataOut, uint8_t* dataIn, uint8_t numBytes);

  // pin number access methods

  /*!
    \brief Access method to get the pin number of SPI chip select.

    \returns Pin number of SPI chip select configured in the constructor.
  */
  RADIOLIB_PIN_TYPE getCs() const { return(_cs); }

  /*!
    \brief Access method to get the pin number of interrupt/GPIO.

    \returns Pin number of interrupt/GPIO configured in the constructor.
  */
  RADIOLIB_PIN_TYPE getIrq() const { return(_irq); }

  /*!
    \brief Access method to get the pin number of hardware reset pin.

    \returns Pin number of hardware reset pin configured in the constructor.
  */
  RADIOLIB_PIN_TYPE getRst() const { return(_rst); }

  /*!
    \brief Access method to get the pin number of second interrupt/GPIO.

    \returns Pin number of second interrupt/GPIO configured in the constructor.
  */
  RADIOLIB_PIN_TYPE getGpio() const { return(_gpio); }

  /*!
    \brief Some modules contain external RF switch controlled by two pins. This function gives RadioLib control over those two pins to automatically switch Rx and Tx state.
    When using automatic RF switch control, DO NOT change the pin mode of rxEn or txEn from Arduino sketch!

    \param rxEn RX enable pin.

    \param txEn TX enable pin.
  */
  void setRfSwitchPins(RADIOLIB_PIN_TYPE rxEn, RADIOLIB_PIN_TYPE txEn);

  /*!
    \brief Set RF switch state.

    \param rxPinState Pin state to set on Tx enable pin (usually high to transmit).

    \param txPinState  Pin state to set on Rx enable pin (usually high to receive).
  */
  void setRfSwitchState(RADIOLIB_PIN_STATUS rxPinState, RADIOLIB_PIN_STATUS txPinState);

  // Arduino core overrides

  /*!
    \brief Arduino core pinMode override that checks RADIOLIB_NC as alias for unused pin.

    \param pin Pin to change the mode of.

    \param mode Which mode to set.
  */
  void pinMode(RADIOLIB_PIN_TYPE pin, RADIOLIB_PIN_MODE mode);

  /*!
    \brief Arduino core digitalWrite override that checks RADIOLIB_NC as alias for unused pin.

    \param pin Pin to write to.

    \param value Whether to set the pin high or low.
  */
  void digitalWrite(RADIOLIB_PIN_TYPE pin, RADIOLIB_PIN_STATUS value);

  /*!
    \brief Arduino core digitalWrite override that checks RADIOLIB_NC as alias for unused pin.

    \param pin Pin to read from.

    \returns Pin value.
  */
  RADIOLIB_PIN_STATUS digitalRead(RADIOLIB_PIN_TYPE pin);

  /*!
    \brief Arduino core tone override that checks RADIOLIB_NC as alias for unused pin and RADIOLIB_TONE_UNSUPPORTED to make sure the platform does support tone.

    \param pin Pin to write to.

    \param value Frequency to output.
  */
  void tone(RADIOLIB_PIN_TYPE pin, uint16_t value, uint32_t duration = 0);

  /*!
    \brief Arduino core noTone override that checks RADIOLIB_NC as alias for unused pin and RADIOLIB_TONE_UNSUPPORTED to make sure the platform does support tone.

    \param pin Pin to write to.
  */
  void noTone(RADIOLIB_PIN_TYPE pin);

  /*!
    \brief Arduino core attachInterrupt override.

    \param interruptNum Interrupt number.

    \param userFunc Interrupt service routine.

    \param mode Pin hcange direction.
  */
  void attachInterrupt(RADIOLIB_PIN_TYPE interruptNum, void (*userFunc)(void), RADIOLIB_INTERRUPT_STATUS mode);

  /*!
    \brief Arduino core detachInterrupt override.

    \param interruptNum Interrupt number.
  */
  void detachInterrupt(RADIOLIB_PIN_TYPE interruptNum);

  /*!
    \brief Arduino core yield override.
  */
  void yield();

  /*!
    \brief Arduino core delay override.

    \param ms Delay length in milliseconds.
  */
  void delay(uint32_t ms);

  /*!
    \brief Arduino core delayMicroseconds override.

    \param us Delay length in microseconds.
  */
  void delayMicroseconds(uint32_t us);

  /*!
    \brief Arduino core millis override.
  */
  uint32_t millis();

  /*!
    \brief Arduino core micros override.
  */
  uint32_t micros();

  /*!
    \brief Arduino core SPI begin override.
  */
  void begin();

  /*!
    \brief Arduino core SPI beginTransaction override.
  */
  void beginTransaction();

  /*!
    \brief Arduino core SPI transfer override.
  */
  uint8_t transfer(uint8_t b);

  /*!
    \brief Arduino core SPI endTransaction override.
  */
  void endTransaction();

  /*!
    \brief Arduino core SPI end override.
  */
  void end();

  // helper functions to set up SPI overrides on Arduino
  void SPIbegin();
  void SPIbeginTransaction();
  uint8_t SPItransfer(uint8_t b);
  void SPIendTransaction();
  void SPIend();

  /*!
    \brief Function to reflect bits within a byte.
  */
  static uint8_t flipBits(uint8_t b);

  /*!
    \brief Function to reflect bits within an integer.
  */
  static uint16_t flipBits16(uint16_t i);

  // hardware abstraction layer callbacks
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_PIN_MODE);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_DIGITAL_WRITE);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_DIGITAL_READ);
  //RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_TONE);
  //RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_NO_TONE);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_ATTACH_INTERRUPT);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_DETACH_INTERRUPT);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_YIELD);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_DELAY);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_DELAY_MICROSECONDS);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_MILLIS);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_MICROS);


  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_SPI_BEGIN);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_SPI_TRANSFER);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_SPI_END_TRANSACTION);
  RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_SPI_END);

#if !defined(RADIOLIB_GODMODE)
private:
#endif

  // pins
  RADIOLIB_PIN_TYPE _cs = RADIOLIB_NC;
  RADIOLIB_PIN_TYPE _irq = RADIOLIB_NC;
  RADIOLIB_PIN_TYPE _rst = RADIOLIB_NC;
  RADIOLIB_PIN_TYPE _gpio = RADIOLIB_NC;


  SPISettings _spiSettings = RADIOLIB_DEFAULT_SPI_SETTINGS;
  SPIClass* _spi = new SPIClass(_spiSettings);

  bool _initInterface = false;

  // RF switch presence and pins
  bool _useRfSwitch = false;
  RADIOLIB_PIN_TYPE _rxEn = RADIOLIB_NC;
  RADIOLIB_PIN_TYPE _txEn = RADIOLIB_NC;
};

#endif
