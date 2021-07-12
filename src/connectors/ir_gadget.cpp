#include "ir_gadget.h"

static const char *TAG = "IR_Gadget";

IR_Gadget::IR_Gadget(int ir_recv_pin, int ir_send_pin) :
    Code_Gadget() {
  bool everything_ok = true;
  if (ir_recv_pin != 0) {
    receiver_ = new IRrecv(ir_recv_pin);
    receiver_->enableIRIn();
    logger_i(TAG, "Receiver-Pin: %d", ir_recv_pin);
  } else {
    everything_ok = false;
    logger_e(TAG, "'recv_pin' not specified.");
  }
  if (ir_send_pin != 0) {
    blaster_ = new IRsend(ir_send_pin);
    blaster_->begin();
    logger_i(TAG, "Blaster-Pin: %d", ir_send_pin);
  } else {
    everything_ok = false;
    logger_e(TAG, "'send_pin' not specified.", ir_recv_pin);
  }
  code_gadget_is_ready_ = everything_ok;
};

void IR_Gadget::refresh() {
  if (!code_gadget_is_ready_) {
    return;
  }
  decode_results results{};
  if (receiver_->decode(&results)) {
    unsigned long received_command{};
    int8_t command_type{};
    command_type = results.decode_type;
    received_command = results.value;
    receiver_->resume();
    if (received_command != 0xFFFFFFFF) {
      switch (command_type) {
        default:
        case UNKNOWN:
          setCommand(IR_UNKNOWN_C, received_command);
          break;
        case NEC:
          setCommand(IR_NEC_C, received_command);
          break;
        case SONY:
          setCommand(IR_SONY_C, received_command);
          break;
        case RC5:
          setCommand(IR_RC5_C, received_command);
          break;
        case RC6:
          setCommand(IR_RC6_C, received_command);
          break;
        case DISH:
          setCommand(IR_UNKNOWN_C, received_command);
          break;
        case SHARP:
          setCommand(IR_UNKNOWN_C, received_command);
          break;
        case JVC:
          setCommand(IR_UNKNOWN_C, received_command);
          break;
        case SANYO:
          setCommand(IR_UNKNOWN_C, received_command);
          break;
        case MITSUBISHI:
          setCommand(IR_UNKNOWN_C, received_command);
          break;
        case SAMSUNG:
          setCommand(IR_SAMSUNG_C, received_command);
          break;
        case LG:
          setCommand(IR_LG_C, received_command);
          break;
        case WHYNTER:
          setCommand(IR_UNKNOWN_C, received_command);
          break;
        case AIWA_RC_T501:
          setCommand(IR_UNKNOWN_C, received_command);
          break;
        case PANASONIC:
          setCommand(IR_PANASONIC_C, received_command);
          break;
        case DENON:
          setCommand(IR_DENON_C, received_command);
          break;
      }
    }
  }
}

bool IR_Gadget::sendRawIR(const uint16_t raw_data[], const uint8_t content_length) {
  if (!code_gadget_is_ready_) {
    return false;
  }
  logger_i(TAG, "Sending Raw Command, 38kHz, %d Blocks", content_length);
  blaster_->sendRaw(raw_data, content_length, 38);
  receiver_->resume();
  return true;
}

bool IR_Gadget::sendIR(unsigned long command, const uint8_t com_type) {
  if (!code_gadget_is_ready_) {
    return false;
  }
  logger_i(TAG, "Sending: ");
  switch (com_type) {
    case NEC:
      blaster_->sendNEC(command);
      break;
    case SONY:
      blaster_->sendSony(command);
      break;
    case SAMSUNG:
      blaster_->sendSAMSUNG(command);
      break;
    case LG:
       blaster_->sendLG(command);
      break;
    case DENON:
      blaster_->sendDenon(command);
      break;
    default:
      logger_i(TAG, "Unsupported Command.");
      return false;
  }
  logger_i(TAG, "Receiver-Pin: %d", command);
  receiver_->resume();
  return true;
}
