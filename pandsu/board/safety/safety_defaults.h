void default_rx_hook(CAN_FIFOMailBox_TypeDef *to_push) {
  int bus = (to_push->RDTR >> 4) & 0xF;
  // 0x5AA send by eon.
  if ((to_push->RIR>>21) == 0x5AA && (bus == 0)) {
    eon_alive = 1;
}

int default_ign_hook() {
  return -1; // use GPIO to determine ignition
}

// *** no output safety mode ***

static void nooutput_init(int16_t param) {
  controls_allowed = 0;
}

static int nooutput_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  return true;//false;
}

static int nooutput_tx_lin_hook(int lin_num, uint8_t *data, int len) {
  return false;
}

static int nooutput_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {
  if ((bus_num == 0 || bus_num == 2)) {
    int addr = to_fwd->RIR>>21;
    bool is_acc_msg = ((eon_alive) && (addr == 0x343) && (bus_num == 2));  //if eon alive block acc msg
    return is_acc_msg? -1 : (uint8_t)(~bus_num & 0x2);
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
  eon_alive=0;
}

static int alloutput_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  return true;
}

static int alloutput_tx_lin_hook(int lin_num, uint8_t *data, int len) {
  return true;
}

static int alloutput_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {
  if ((bus_num == 0 || bus_num == 2)) {
    int addr = to_fwd->RIR>>21;
    bool is_acc_msg = ((eon_alive) && (addr == 0x343) && (bus_num == 2));
    return is_acc_msg? -1 : (uint8_t)(~bus_num & 0x2);
  }
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
