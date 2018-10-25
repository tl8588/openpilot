void default_rx_hook(CAN_FIFOMailBox_TypeDef *to_push) {}

int default_ign_hook() {
  return -1; // use GPIO to determine ignition
}

// *** no output safety mode ***

static void nooutput_init(int16_t param) {
  controls_allowed = 0;
}

static int nooutput_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  return false;
}

static int nooutput_tx_lin_hook(int lin_num, uint8_t *data, int len) {
  return false;
}

static int nooutput_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {
  if ((bus_num == 0 || bus_num == 2) && !toyota_giraffe_switch_1) {
    int addr = to_fwd->RIR>>21;
    bool is_lkas_msg = (addr == 0x2E4 || addr == 0x412) && bus_num == 2;
    return is_lkas_msg? -1 : (uint8_t)(~bus_num & 0x2);
  }
  return -1;
}

const safety_hooks nooutput_hooks = {
  .init = nooutput_init,
  .rx = default_rx_hook,
  .tx = nooutput_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .ignition = default_ign_hook,
  .fwd = nooutput_fwd_hook,
};

// *** all output safety mode ***

static void alloutput_init(int16_t param) {
  controls_allowed = 1;
}

static int alloutput_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  return true;
}

static int alloutput_tx_lin_hook(int lin_num, uint8_t *data, int len) {
  return true;
}

static int alloutput_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {
  return -1;
}

const safety_hooks alloutput_hooks = {
  .init = alloutput_init,
  .rx = default_rx_hook,
  .tx = alloutput_tx_hook,
  .tx_lin = alloutput_tx_lin_hook,
  .ignition = default_ign_hook,
  .fwd = alloutput_fwd_hook,
};
