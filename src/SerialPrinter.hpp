#pragma once

#include <lgfx/v1/platforms/arduino_default/Bus_Stream.hpp>

namespace lgfx_addon
{
//----------------------------------------------------------------------------

  template <typename T>
  class SerialPrinter : public lgfx::LGFX_Device
  {
    T                 _panel_instance;
    lgfx::Bus_Stream  _bus_instance;

  public:
    SerialPrinter(Stream* stream, uint16_t height, int rts) : SerialPrinter(stream, 0, height, rts) {;}

    SerialPrinter(Stream* stream, uint16_t width, uint16_t height, int rts)
    {
      {
        auto cfg = _bus_instance.config();
        cfg.stream = stream;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
      }
      {
        auto cfg = _panel_instance.config();
        cfg.pin_busy = rts;
        if(width > 0)
          cfg.memory_width  = cfg.panel_width  = width;
        if(height > 0)
          cfg.memory_height = cfg.panel_height = height;
        _panel_instance.config(cfg);
      }
      setPanel(&_panel_instance);
    }
  };

  //----------------------------------------------------------------------------
}
