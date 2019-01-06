int toyota_no_dsu_car = 0;                // ch-r and camry don't have the DSU
int toyota_giraffe_switch_1 = 0;          // is giraffe switch 1 high?
uint32_t eon_tmr = 0;
//int eon_alive = 0;
// global torque limit
const int TOYOTA_MAX_TORQUE = 1500;       // max torque cmd allowed ever

// rate based torque limit + stay within actually applied
// packet is sent at 100hz, so this limit is 1000/sec
const int TOYOTA_MAX_RATE_UP = 10;        // ramp up slow
const int TOYOTA_MAX_RATE_DOWN = 25;      // ramp down fast
const int TOYOTA_MAX_TORQUE_ERROR = 350;  // max torque cmd in excess of torque motor

// real time torque limit to prevent controls spamming
// the real time limit is 1500/sec
const int TOYOTA_MAX_RT_DELTA = 375;      // max delta torque allowed for real time checks
const int TOYOTA_RT_INTERVAL = 250000;    // 250ms between real time checks

// longitudinal limits
const int TOYOTA_MAX_ACCEL = 1500;        // 1.5 m/s2
const int TOYOTA_MIN_ACCEL = -3000;       // 3.0 m/s2

// global actuation limit state
int toyota_actuation_limits = 1;          // by default steer limits are imposed
int toyota_dbc_eps_torque_factor = 100;   // conversion factor for STEER_TORQUE_EPS in %: see dbc file

// state of torque limits
int toyota_desired_torque_last = 0;       // last desired steer torque
int toyota_rt_torque_last = 0;            // last desired torque for real time check
uint32_t toyota_ts_last = 0;
int toyota_cruise_engaged_last = 0;       // cruise state
struct sample_t toyota_torque_meas;       // last 3 motor torques produced by the eps


static void toyota_rx_hook(CAN_FIFOMailBox_TypeDef *to_push) {
  // get eps motor torque (0.66 factor in dbc)
  if ((to_push->RIR>>21) == 0x260) {
    int torque_meas_new = (((to_push->RDHR) & 0xFF00) | ((to_push->RDHR >> 16) & 0xFF));
    torque_meas_new = to_signed(torque_meas_new, 16);

    // scale by dbc_factor
    torque_meas_new = (torque_meas_new * toyota_dbc_eps_torque_factor) / 100;

    // increase torque_meas by 1 to be conservative on rounding
    torque_meas_new += (torque_meas_new > 0 ? 1 : -1);

    // update array of sample
    update_sample(&toyota_torque_meas, torque_meas_new);
  }

  // enter controls on rising edge of ACC, exit controls on ACC off
  if ((to_push->RIR>>21) == 0x1D2) {
    // 4 bits: 55-52
    int cruise_engaged = to_push->RDHR & 0xF00000;
    if (cruise_engaged && !toyota_cruise_engaged_last) {
      controls_allowed = 1;
    } else if (!cruise_engaged) {
      controls_allowed = 0;
    }
    toyota_cruise_engaged_last = cruise_engaged;
  }

  int bus = (to_push->RDTR >> 4) & 0xF;
  // 0x680 is a radar msg only found in dsu-less cars
  if ((to_push->RIR>>21) == 0x680 && (bus == 1)) {
    toyota_no_dsu_car = 1;
  }

  // 0x5AA send by eon.
  if ((to_push->RIR>>21) == 0x5AA && (bus == 0)) {
    eon_alive = 1;
    eon_tmr=TIM2->CNT;
  }
  uint32_t ts_elapsed = get_ts_elapsed(TIM2->CNT, eon_tmr);
  if (ts_elapsed > 1200000) // >1.2s
  {
     eon_alive = 0;
  }
}

static int toyota_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {

  // 1 allows the message through
  return true;
}

static void toyota_init(int16_t param) {
  controls_allowed = 0;
  //eon_alive = 0;
  toyota_actuation_limits = 1;
  toyota_giraffe_switch_1 = 0;
  toyota_dbc_eps_torque_factor = param;
}

static int toyota_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {

  if ((bus_num == 0 || bus_num == 2)) {
    int addr = to_fwd->RIR>>21;

    bool is_acc_msg = ((eon_alive==1) && (addr == 0x343) && (bus_num == 2));
    return is_acc_msg? -1 : (uint8_t)(~bus_num & 0x2);
  }
  return -1;
}

const safety_hooks toyota_hooks = {
  .init = toyota_init,
  .rx = toyota_rx_hook,
  .tx = toyota_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .ignition = default_ign_hook,
  .fwd = toyota_fwd_hook,
};

static void toyota_nolimits_init(int16_t param) {
  controls_allowed = 0;
  toyota_actuation_limits = 0;
  toyota_giraffe_switch_1 = 0;
  toyota_dbc_eps_torque_factor = param;
}

const safety_hooks toyota_nolimits_hooks = {
  .init = toyota_nolimits_init,
  .rx = toyota_rx_hook,
  .tx = toyota_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .ignition = default_ign_hook,
  .fwd = toyota_fwd_hook,
};
