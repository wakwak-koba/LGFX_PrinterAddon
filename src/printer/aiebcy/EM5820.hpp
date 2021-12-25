#pragma once

#include "../escpos.hpp"

namespace lgfx_addon
{
 inline namespace aiebcy
 {
//----------------------------------------------------------------------------

  struct EM5820 : public lgfx_addon::epson::ESCPOS
  {
    EM5820(void) : ESCPOS(384) {;}

  	void display(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h) override { print_GS_v(x, y, w, h); }
  };

//----------------------------------------------------------------------------
 }
}
