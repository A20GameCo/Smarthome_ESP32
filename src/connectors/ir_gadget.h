#pragma once

#ifdef UNIT_TEST
#include "../test_dummys/ir_dummy.h"
#else
#include <IRrecv.h>
#include <IRsend.h>
#endif //UNIT_TEST

#include "code_gadget.h"

// Gadget to send and receive IR-Commands
class IR_Gadget : public Code_Gadget {
protected:

  IRrecv *receiver_;
  IRsend *blaster_;

public:

  explicit IR_Gadget(int ir_recv_pin, int ir_send_pin);

  void refresh() override;

  bool sendRawIR(const uint16_t raw_data[], uint8_t content_length);

  bool sendIR(unsigned long command, uint8_t com_type);
};
