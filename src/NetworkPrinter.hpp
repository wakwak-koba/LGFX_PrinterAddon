#pragma once

#include <lgfx/v1/platforms/arduino_default/Bus_Stream.hpp>

namespace lgfx_addon
{
//----------------------------------------------------------------------------

  template <typename T>
  class NetworkPrinter : public lgfx::LGFX_Device
  {
    T                 _panel_instance;
    lgfx::Bus_Stream  _bus_instance;

  public:
    NetworkPrinter(Stream* stream, uint16_t height) : NetworkPrinter(stream, 0, height) {;}

    NetworkPrinter(Stream* stream, uint16_t width, uint16_t height)
    {
      {
        auto cfg = _bus_instance.config();
        cfg.stream = stream;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
      }
      {
        auto cfg = _panel_instance.config();
        if(width > 0)
          cfg.memory_width  = cfg.panel_width  = width;
        if(height > 0)
          cfg.memory_height = cfg.panel_height = height;
        _panel_instance.config(cfg);
      }
      setPanel(&_panel_instance);
    }
    
    T* getPanel() { return &_panel_instance; }
  };

  //----------------------------------------------------------------------------
}
