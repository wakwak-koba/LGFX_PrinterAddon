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
  };

//----------------------------------------------------------------------------
 }
}
