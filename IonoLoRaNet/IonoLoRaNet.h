#ifndef IonoLoRaNet_h
#define IonoLoRaNet_h

#include <Arduino.h>
#include <LoRaNet.h>

class IonoLoRaLocalMaster : public LoRaLocalMaster {
};

class IonoLoRaRemoteSlave : public LoRaRemoteSlave {
  private:
    bool _cmd_changed;
    byte _cmd_mask;
    byte _cmd_dos;
    uint16_t _cmd_ao1;
    byte _cmd_data[4];
    float _cmd_values[5];
    float _values[21];
    uint16_t _diCount1;
    uint16_t _diCount2;
    uint16_t _diCount3;
    uint16_t _diCount4;
    uint16_t _diCount5;
    uint16_t _diCount6;

    bool _has_cmds();
    byte* _get_cmd_data();
    int _get_cmd_data_len();
    bool _check_cmd_success();

  public:
    IonoLoRaRemoteSlave();
    IonoLoRaRemoteSlave(byte unitAddr);
    void write(int pin, float value);
    float read(int pin);
    uint16_t diCount(int pin);
    void _update_state(byte *data, int data_len);
};

class IonoLoRaLocalSlave : public LoRaLocalSlave {
  private:
    static IonoLoRaLocalSlave *_INSTANCE;
    bool _state_changed;
    byte _state_modes;
    byte _state_dos;
    uint16_t _state_ao1;
    byte _state_dis;
    uint16_t _state_a1;
    uint16_t _state_a2;
    uint16_t _state_a3;
    uint16_t _state_a4;
    uint16_t _state_c1;
    uint16_t _state_c2;
    uint16_t _state_c3;
    uint16_t _state_c4;
    uint16_t _state_c5;
    uint16_t _state_c6;
    byte _state_data[25];
    bool _in1_first_change;
    unsigned long _in1_last_change;
    unsigned long _in1_min_delay;
    bool _in2_first_change;
    unsigned long _in2_last_change;
    unsigned long _in2_min_delay;
    bool _in3_first_change;
    unsigned long _in3_last_change;
    unsigned long _in3_min_delay;
    bool _in4_first_change;
    unsigned long _in4_last_change;
    unsigned long _in4_min_delay;
    bool _in5_first_change;
    unsigned long _in5_last_change;
    unsigned long _in5_min_delay;
    bool _in6_first_change;
    unsigned long _in6_last_change;
    unsigned long _in6_min_delay;

    bool _has_updates();
    byte* _get_state_data();
    int _get_state_data_len();
    void _process_ios();

  public:
    IonoLoRaLocalSlave();
    IonoLoRaLocalSlave(byte unitAddr);
    void setUpdatesInterval(uint8_t pin, unsigned long seconds);
    static void subscribeCallback(uint8_t pin, float value);
    void _set_state(byte *data, int data_len);
};

#endif
