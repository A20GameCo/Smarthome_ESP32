#pragma once

#include "sh_lamp.h"

// UNKNOWN 19496A87
static const uint16_t lamp_on[143] = {1246, 434, 1248, 432, 408, 1300, 382, 1300, 380, 1298, 384, 1296, 382,
                                      1298, 382, 1298, 1220,
                                      432, 410, 1300, 380, 1298, 382, 8004, 1248, 430, 1250, 432, 408, 1298,
                                      382, 1300, 382, 1298,
                                      382, 1300, 382, 1298, 382, 1298, 1222, 434, 408, 1300, 380, 1298, 390,
                                      8000, 1244, 432, 1248,
                                      432, 410, 1300, 380, 1300, 382, 1298, 382, 1300, 382, 1298, 382, 1298,
                                      1220, 434, 408, 1300,
                                      380, 1298, 382, 8004, 1248, 432, 1248, 432, 410, 1300, 380, 1300, 382,
                                      1298, 382, 1300, 382,
                                      1298, 380, 1298, 1222, 434, 408, 1300, 382, 1298, 382, 8006, 1246,
                                      432, 1248, 432, 408, 1300,
                                      380, 1300, 382, 1298, 382, 1300, 382, 1298, 384, 1296, 1220, 434, 408,
                                      1298, 382, 1298, 382,
                                      8004, 1246, 434, 1248, 432, 408, 1298, 382, 1300, 382, 1298, 382,
                                      1300, 380, 1298, 382, 1298,
                                      1220, 434, 410, 1298, 382, 1298, 382};  // UNKNOWN 666F143F


// UNKNOWN ACF483A9
static const uint16_t lamp_off[119] = {1248, 432, 1250, 434, 408, 1298, 382, 1300, 380, 1298, 382, 1296,
                                       1222, 434, 408, 1300, 382,
                                       1298, 382, 1298, 384, 1296, 382, 8030, 1220, 460, 1220, 460, 380,
                                       1300, 380, 1300, 380,
                                       1300, 380, 1298, 1222, 462, 380, 1300, 382, 1298, 382, 1300, 382,
                                       1298, 382, 8002, 1250,
                                       460, 1218, 462, 380, 1298, 382, 1300, 382, 1298, 382, 1298, 1220,
                                       462, 380, 1300, 380, 1298,
                                       382, 1298, 382, 1298, 382, 8002, 1248, 460, 1220, 460, 380, 1298,
                                       382, 1300, 382, 1296, 384,
                                       1296, 1222, 462, 382, 1298, 382, 1298, 380, 1300, 382, 1298, 382,
                                       8006, 1246, 458, 1220,
                                       462, 380, 1298, 382, 1300, 382, 1298, 380, 1298, 1222, 462, 380,
                                       1298, 382, 1298, 380, 1298,
                                       382, 1298, 382};  // UNKNOWN 94024B86

class SH_Lamp_Westinghouse_IR : public SH_Lamp {
protected:

public:

  explicit SH_Lamp_Westinghouse_IR(const JsonObject&);

  void refresh() override;
};
