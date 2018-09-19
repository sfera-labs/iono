#include "IonoLoRaNet.h"
#include <Iono.h>

IonoLoRaRemoteSlave::IonoLoRaRemoteSlave()
: IonoLoRaRemoteSlave(0xff) {
}

IonoLoRaRemoteSlave::IonoLoRaRemoteSlave(byte unitAddr)
: LoRaRemoteSlave(unitAddr) {
  for (int i = 0; i < 21; i++) {
    _values[i] = -1;
  }

  for (int i = 0; i < 5; i++) {
    _cmd_values[i] = -1;
  }

  _diCount1 = 0;
  _diCount2 = 0;
  _diCount3 = 0;
  _diCount4 = 0;
  _diCount5 = 0;
  _diCount6 = 0;

  _cmd_changed = false;
}

bool IonoLoRaRemoteSlave::_check_cmd_success() {
  if (_cmd_values[0] != -1 && _cmd_values[0] != _values[DO1]) {
    return false;
  }

  if (_cmd_values[1] != -1 && _cmd_values[1] != _values[DO2]) {
    return false;
  }

  if (_cmd_values[2] != -1 && _cmd_values[2] != _values[DO3]) {
    return false;
  }

  if (_cmd_values[3] != -1 && _cmd_values[3] != _values[DO4]) {
    return false;
  }

  if (_cmd_values[4] != -1 && _cmd_values[4] != _values[AO1]) {
    return false;
  }

  return true;
}

void IonoLoRaRemoteSlave::_update_state(byte *data, int data_len) {
  byte modes_byte = data[0];
  byte dos = data[1];
  uint16_t ao1 = ((data[2] & 0xff) << 8) | (data[3] & 0xff);
  byte dis = data[4];
  uint16_t a1 = ((data[5] & 0xff) << 8) | (data[6] & 0xff);
  uint16_t a2 = ((data[7] & 0xff) << 8) | (data[8] & 0xff);
  uint16_t a3 = ((data[9] & 0xff) << 8) | (data[10] & 0xff);
  uint16_t a4 = ((data[11] & 0xff) << 8) | (data[12] & 0xff);
  _diCount1 = ((data[13] & 0xff) << 8) | (data[14] & 0xff);
  _diCount2 = ((data[15] & 0xff) << 8) | (data[16] & 0xff);
  _diCount3 = ((data[17] & 0xff) << 8) | (data[18] & 0xff);
  _diCount4 = ((data[19] & 0xff) << 8) | (data[20] & 0xff);
  _diCount5 = ((data[21] & 0xff) << 8) | (data[22] & 0xff);
  _diCount6 = ((data[23] & 0xff) << 8) | (data[24] & 0xff);

  int mode_di1 = (modes_byte >> 7) & 1;
  int mode_di2 = (modes_byte >> 5) & 1;
  int mode_di3 = (modes_byte >> 3) & 1;
  int mode_di4 = (modes_byte >> 1) & 1;

  int mode_a1 = (modes_byte >> 6) & 1;
  int mode_a2 = (modes_byte >> 4) & 1;
  int mode_a3 = (modes_byte >> 2) & 1;
  int mode_a4 = modes_byte & 1;

  float a1f, a2f, a3f, a4f;

  if (a1 == 0xffff) {
    a1f = -1;
  } else {
    a1f = a1 / 1000.0;
  }

  if (a2 == 0xffff) {
    a2f = -1;
  } else {
    a2f = a2 / 1000.0;
  }

  if (a3 == 0xffff) {
    a3f = -1;
  } else {
    a3f = a3 / 1000.0;
  }

  if (a4 == 0xffff) {
    a4f = -1;
  } else {
    a4f = a4 / 1000.0;
  }

  _values[DO1] = (dos >> 3) & 1;
  _values[DO2] = (dos >> 2) & 1;
  _values[DO3] = (dos >> 1) & 1;
  _values[DO4] = dos & 1;

  _values[AO1] = ao1 / 1000.0;

  _values[DI1] = mode_di1 == 1 ? (dis >> 5) & 1 : -1;
  _values[DI2] = mode_di2 == 1 ? (dis >> 4) & 1 : -1;
  _values[DI3] = mode_di3 == 1 ? (dis >> 3) & 1 : -1;
  _values[DI4] = mode_di4 == 1 ? (dis >> 2) & 1 : -1;
  _values[DI5] = (dis >> 1) & 1;
  _values[DI6] = dis & 1;

  if (mode_a1 == 0) {
    _values[AV1] = a1f;
    _values[AI1] = -1;
  } else {
    _values[AV1] = -1;
    _values[AI1] = a1f;
  }

  if (mode_a2 == 0) {
    _values[AV2] = a2f;
    _values[AI2] = -1;
  } else {
    _values[AV2] = -1;
    _values[AI2] = a2f;
  }

  if (mode_a3 == 0) {
    _values[AV3] = a3f;
    _values[AI3] = -1;
  } else {
    _values[AV3] = -1;
    _values[AI3] = a3f;
  }

  if (mode_a4 == 0) {
    _values[AV4] = a4f;
    _values[AI4] = -1;
  } else {
    _values[AV4] = -1;
    _values[AI4] = a4f;
  }
}

bool IonoLoRaRemoteSlave::_has_cmds() {
  return _cmd_changed;
}

byte* IonoLoRaRemoteSlave::_get_cmd_data() {
  _cmd_changed = false;
  _cmd_data[0] = _cmd_mask;
  _cmd_data[1] = _cmd_dos;
  _cmd_data[2] = (byte) ((_cmd_ao1 >> 8) & 0xff);
  _cmd_data[3] = (byte) (_cmd_ao1 & 0xff);
  return _cmd_data;
}

int IonoLoRaRemoteSlave::_get_cmd_data_len() {
  return 4;
}

void IonoLoRaRemoteSlave::write(int pin, float value) {
  int idx;
  switch (pin) {
    case DO1:
      if (value != HIGH && value != LOW) {
        return;
      }
      idx = 0;
      _cmd_mask |= 0x10;
      _cmd_dos = (_cmd_dos & 0b0111) | ((value == HIGH ? 1 : 0) << 3);
      break;

    case DO2:
      if (value != HIGH && value != LOW) {
        return;
      }
      idx = 1;
      _cmd_mask |= 0x08;
      _cmd_dos = (_cmd_dos & 0b1011) | ((value == HIGH ? 1 : 0) << 2);
      break;

    case DO3:
      if (value != HIGH && value != LOW) {
        return;
      }
      idx = 2;
      _cmd_mask |= 0x04;
      _cmd_dos = (_cmd_dos & 0b1101) | ((value == HIGH ? 1 : 0) << 1);
      break;

    case DO4:
      if (value != HIGH && value != LOW) {
        return;
      }
      idx = 3;
      _cmd_mask |= 0x02;
      _cmd_dos = (_cmd_dos & 0b1110) | (value == HIGH ? 1 : 0);
      break;

    case AO1:
      if (value < 0 || value > 10) {
        return;
      }
      idx = 4;
      _cmd_mask |= 0x01;
      _cmd_ao1 = (uint16_t) (value * 1000);
      break;

    default:
      return;
  }

  _cmd_values[idx] = value;
  _cmd_changed = true;
}

float IonoLoRaRemoteSlave::read(int pin) {
  return _values[pin];
}

uint16_t IonoLoRaRemoteSlave::diCount(int pin) {
  switch (pin) {
    case DI1:
      return _diCount1;
    case DI2:
      return _diCount2;
    case DI3:
      return _diCount3;
    case DI4:
      return _diCount4;
    case DI5:
      return _diCount5;
    case DI6:
      return _diCount6;
    default:
      return 0;
  }
}
