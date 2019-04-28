#include "IonoLoRaNet.h"
#include <Iono.h>

IonoLoRaLocalSlave *IonoLoRaLocalSlave::_INSTANCE;

IonoLoRaLocalSlave::IonoLoRaLocalSlave()
: IonoLoRaLocalSlave(0xff) {
}

IonoLoRaLocalSlave::IonoLoRaLocalSlave(byte unitAddr)
: LoRaLocalSlave(unitAddr) {
  _INSTANCE = this;
  _state_changed = true;
  _state_modes = 0x00;
  _state_dos = 0x00;
  _state_ao1 = 0xffff;
  _state_dis = 0x00;
  _state_a1 = 0xffff;
  _state_a2 = 0xffff;
  _state_a3 = 0xffff;
  _state_a4 = 0xffff;
  _state_c1 = 0;
  _state_c2 = 0;
  _state_c3 = 0;
  _state_c4 = 0;
  _state_c5 = 0;
  _state_c6 = 0;
  _in1_first_change = true;
  _in1_last_change = 0;
  _in1_min_delay = 0;
  _in2_first_change = true;
  _in2_last_change = 0;
  _in2_min_delay = 0;
  _in3_first_change = true;
  _in3_last_change = 0;
  _in3_min_delay = 0;
  _in4_first_change = true;
  _in4_last_change = 0;
  _in4_min_delay = 0;
  _in5_first_change = true;
  _in5_last_change = 0;
  _in5_min_delay = 0;
  _in6_first_change = true;
  _in6_last_change = 0;
  _in6_min_delay = 0;
}

void IonoLoRaLocalSlave::_process_ios() {
  Iono.process();
}

bool IonoLoRaLocalSlave::_has_updates() {
  return _state_changed;
}

int IonoLoRaLocalSlave::_get_state_data_len() {
  return 25;
}

void IonoLoRaLocalSlave::_set_state(byte *data, int data_len) {
  byte mask = data[0];
  byte dos = data[1];
  uint16_t ao1 = ((data[2] & 0xff) << 8) | (data[3] & 0xff);

  if ((mask >> 4) & 1 == 1) {
    Iono.write(DO1, (dos >> 3) & 1);
  }

  if ((mask >> 3) & 1 == 1) {
    Iono.write(DO2, (dos >> 2) & 1);
  }

  if ((mask >> 2) & 1 == 1) {
    Iono.write(DO3, (dos >> 1) & 1);
  }

  if ((mask >> 1) & 1 == 1) {
    Iono.write(DO4, dos & 1);
  }

  if (mask & 1 == 1) {
    Iono.write(AO1, ao1 / 1000.0);
  }
}

byte* IonoLoRaLocalSlave::_get_state_data() {
  _state_changed = false;
  _state_data[0] = _state_modes;
  _state_data[1] = _state_dos;
  _state_data[2] = (byte) ((_state_ao1 >> 8) & 0xff);
  _state_data[3] = (byte) (_state_ao1 & 0xff);
  _state_data[4] = _state_dis;
  _state_data[5] = (byte) ((_state_a1 >> 8) & 0xff);
  _state_data[6] = (byte) (_state_a1 & 0xff);
  _state_data[7] = (byte) ((_state_a2 >> 8) & 0xff);
  _state_data[8] = (byte) (_state_a2 & 0xff);
  _state_data[9] = (byte) ((_state_a3 >> 8) & 0xff);
  _state_data[10] = (byte) (_state_a3 & 0xff);
  _state_data[11] = (byte) ((_state_a4 >> 8) & 0xff);
  _state_data[12] = (byte) (_state_a4 & 0xff);
  _state_data[13] = (byte) ((_state_c1 >> 8) & 0xff);
  _state_data[14] = (byte) (_state_c1 & 0xff);
  _state_data[15] = (byte) ((_state_c2 >> 8) & 0xff);
  _state_data[16] = (byte) (_state_c2 & 0xff);
  _state_data[17] = (byte) ((_state_c3 >> 8) & 0xff);
  _state_data[18] = (byte) (_state_c3 & 0xff);
  _state_data[19] = (byte) ((_state_c4 >> 8) & 0xff);
  _state_data[20] = (byte) (_state_c4 & 0xff);
  _state_data[21] = (byte) ((_state_c5 >> 8) & 0xff);
  _state_data[22] = (byte) (_state_c5 & 0xff);
  _state_data[23] = (byte) ((_state_c6 >> 8) & 0xff);
  _state_data[24] = (byte) (_state_c6 & 0xff);
  return _state_data;
}

void IonoLoRaLocalSlave::setUpdatesInterval(uint8_t pin, unsigned long seconds) {
  switch (pin) {
    case DI1:
    case AV1:
    case AI1:
      _in1_min_delay = seconds * 1000;
      break;
    case DI2:
    case AV2:
    case AI2:
      _in2_min_delay = seconds * 1000;
      break;
    case DI3:
    case AV3:
    case AI3:
      _in3_min_delay = seconds * 1000;
      break;
    case DI4:
    case AV4:
    case AI4:
      _in4_min_delay = seconds * 1000;
      break;
    case DI5:
      _in5_min_delay = seconds * 1000;
      break;
    case DI6:
      _in6_min_delay = seconds * 1000;
      break;
  }
}

void IonoLoRaLocalSlave::subscribeCallback(uint8_t pin, float value) {
  int dVal;
  unsigned long now = millis();

  switch (pin) {
    case DO1:
      dVal = value == HIGH ? 1 : 0;
      _INSTANCE->_state_dos = (_INSTANCE->_state_dos & 0b0111) | (dVal << 3);
      _INSTANCE->_state_changed = true;
      break;

    case DO2:
      dVal = value == HIGH ? 1 : 0;
      _INSTANCE->_state_dos = (_INSTANCE->_state_dos & 0b1011) | (dVal << 2);
      _INSTANCE->_state_changed = true;
      break;

    case DO3:
      dVal = value == HIGH ? 1 : 0;
      _INSTANCE->_state_dos = (_INSTANCE->_state_dos & 0b1101) | (dVal << 1);
      _INSTANCE->_state_changed = true;
      break;

    case DO4:
      dVal = value == HIGH ? 1 : 0;
      _INSTANCE->_state_dos = (_INSTANCE->_state_dos & 0b1110) | dVal;
      _INSTANCE->_state_changed = true;
      break;

    case AO1:
      _INSTANCE->_state_ao1 = (uint16_t) (value * 1000);
      _INSTANCE->_state_changed = true;
      break;

    case DI1:
      _INSTANCE->_state_modes |= 0b10000000;
      if (_INSTANCE->_in1_first_change || now - _INSTANCE->_in1_last_change >= _INSTANCE->_in1_min_delay) {
        _INSTANCE->_in1_first_change = false;
        _INSTANCE->_in1_last_change = now;
        dVal = value == HIGH ? 1 : 0;
        _INSTANCE->_state_dis = (_INSTANCE->_state_dis & 0b011111) | (dVal << 5);
        if (dVal) {
          _INSTANCE->_state_c1++;
        }
        _INSTANCE->_state_changed = true;
      }
      break;

    case DI2:
      _INSTANCE->_state_modes |= 0b00100000;
      if (_INSTANCE->_in2_first_change || now - _INSTANCE->_in2_last_change >= _INSTANCE->_in2_min_delay) {
        _INSTANCE->_in2_first_change = false;
        _INSTANCE->_in2_last_change = now;
        dVal = value == HIGH ? 1 : 0;
        _INSTANCE->_state_dis = (_INSTANCE->_state_dis & 0b101111) | (dVal << 4);
        if (dVal) {
          _INSTANCE->_state_c2++;
        }
        _INSTANCE->_state_changed = true;
      }
      break;

    case DI3:
      _INSTANCE->_state_modes |= 0b00001000;
      if (_INSTANCE->_in3_first_change || now - _INSTANCE->_in3_last_change >= _INSTANCE->_in3_min_delay) {
        _INSTANCE->_in3_first_change = false;
        _INSTANCE->_in3_last_change = now;
        dVal = value == HIGH ? 1 : 0;
        _INSTANCE->_state_dis = (_INSTANCE->_state_dis & 0b110111) | (dVal << 3);
        if (dVal) {
          _INSTANCE->_state_c3++;
        }
        _INSTANCE->_state_changed = true;
      }
      break;

    case DI4:
      _INSTANCE->_state_modes |= 0b00000010;
      if (_INSTANCE->_in4_first_change || now - _INSTANCE->_in4_last_change >= _INSTANCE->_in4_min_delay) {
        _INSTANCE->_in4_first_change = false;
        _INSTANCE->_in4_last_change = now;
        dVal = value == HIGH ? 1 : 0;
        _INSTANCE->_state_dis = (_INSTANCE->_state_dis & 0b111011) | (dVal << 2);
        if (dVal) {
          _INSTANCE->_state_c4++;
        }
        _INSTANCE->_state_changed = true;
      }
      break;

    case DI5:
      if (_INSTANCE->_in5_first_change || now - _INSTANCE->_in5_last_change >= _INSTANCE->_in5_min_delay) {
        _INSTANCE->_in5_first_change = false;
        _INSTANCE->_in5_last_change = now;
        dVal = value == HIGH ? 1 : 0;
        _INSTANCE->_state_dis = (_INSTANCE->_state_dis & 0b111101) | (dVal << 1);
        if (dVal) {
          _INSTANCE->_state_c5++;
        }
        _INSTANCE->_state_changed = true;
      }
      break;

    case DI6:
      if (_INSTANCE->_in6_first_change || now - _INSTANCE->_in6_last_change >= _INSTANCE->_in6_min_delay) {
        _INSTANCE->_in6_first_change = false;
        _INSTANCE->_in6_last_change = now;
        dVal = value == HIGH ? 1 : 0;
        _INSTANCE->_state_dis = (_INSTANCE->_state_dis & 0b111110) | dVal;
        if (dVal) {
          _INSTANCE->_state_c6++;
        }
        _INSTANCE->_state_changed = true;
      }
      break;

    case AI1:
      _INSTANCE->_state_modes |= 0b01000000;
    case AV1:
      if (_INSTANCE->_in1_first_change || now - _INSTANCE->_in1_last_change >= _INSTANCE->_in1_min_delay) {
        _INSTANCE->_in1_first_change = false;
        _INSTANCE->_in1_last_change = now;
        _INSTANCE->_state_a1 = (uint16_t) (value * 1000);
        _INSTANCE->_state_changed = true;
      }
      break;

    case AI2:
      _INSTANCE->_state_modes |= 0b00010000;
    case AV2:
      if (_INSTANCE->_in2_first_change || now - _INSTANCE->_in2_last_change >= _INSTANCE->_in2_min_delay) {
        _INSTANCE->_in2_first_change = false;
        _INSTANCE->_in2_last_change = now;
        _INSTANCE->_state_a2 = (uint16_t) (value * 1000);
        _INSTANCE->_state_changed = true;
      }
      break;

    case AI3:
      _INSTANCE->_state_modes |= 0b00000100;
    case AV3:
      if (_INSTANCE->_in3_first_change || now - _INSTANCE->_in3_last_change >= _INSTANCE->_in3_min_delay) {
        _INSTANCE->_in3_first_change = false;
        _INSTANCE->_in3_last_change = now;
        _INSTANCE->_state_a3 = (uint16_t) (value * 1000);
        _INSTANCE->_state_changed = true;
      }
      break;

    case AI4:
      _INSTANCE->_state_modes |= 0b00000001;
    case AV4:
      if (_INSTANCE->_in4_first_change || now - _INSTANCE->_in4_last_change >= _INSTANCE->_in4_min_delay) {
        _INSTANCE->_in4_first_change = false;
        _INSTANCE->_in4_last_change = now;
        _INSTANCE->_state_a4 = (uint16_t) (value * 1000);
        _INSTANCE->_state_changed = true;
      }
      break;
  }
}
