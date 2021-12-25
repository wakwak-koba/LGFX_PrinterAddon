#pragma once

#include <lgfx/v1/panel/Panel_HasBuffer.hpp>
#include <lgfx/v1/misc/range.hpp>

namespace lgfx_addon
{
 inline namespace epson
 {
//----------------------------------------------------------------------------

  struct ESCPOS : public lgfx::Panel_HasBuffer
  {
    ESCPOS() {;}
    ESCPOS(uint16_t width);
    ESCPOS(uint16_t width, uint16_t height);

    bool init(bool use_reset) override;

    lgfx::color_depth_t setColorDepth(lgfx::color_depth_t depth) override;

    void setInvert(bool invert) override;
    void setSleep(bool flg) override {;}
    void setPowerSave(bool flg) {;}

    void waitDisplay(void) override;
    bool displayBusy(void) override;
    void display(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h) override { print_GS_v(x, y, w, h); }

    void writeFillRectPreclipped(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, uint32_t rawcolor) override;
    void writeImage(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, lgfx::pixelcopy_t* param, bool use_dma) override;
    void writePixels(lgfx::pixelcopy_t* param, uint32_t len, bool use_dma) override;

    uint32_t readCommand(uint_fast8_t, uint_fast8_t, uint_fast8_t) override { return 0; }
    uint32_t readData(uint_fast8_t, uint_fast8_t) override { return 0; }

    void readRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, void* dst, lgfx::pixelcopy_t* param) override;

  private:

    size_t _get_buffer_length(void) const override;

    void writeBytes(const uint8_t* data, uint32_t length);
    bool _wait_busy(uint32_t timeout = 1000);
    void _draw_pixel(uint_fast16_t x, uint_fast16_t y, uint32_t value);
    bool _read_pixel(uint_fast16_t x, uint_fast16_t y);
    
  protected:
    void print_ESC_asterisk(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h);
    void print_GS_v(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h);
    void print_GS_L(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h);
    
  };
//----------------------------------------------------------------------------
 }
}
