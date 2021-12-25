#include "ESCPOS.hpp"
#include <lgfx/v1/Bus.hpp>
#include <lgfx/v1/platforms/common.hpp>
#include <lgfx/v1/misc/pixelcopy.hpp>
#include <lgfx/v1/misc/colortype.hpp>

#ifdef min
#undef min
#endif

namespace lgfx_addon
{
 inline namespace epson
 {
//----------------------------------------------------------------------------

  static constexpr uint8_t Bayer[16] = { 8, 200, 40, 232, 72, 136, 104, 168, 56, 248, 24, 216, 120, 184, 88, 152 };

  ESCPOS::ESCPOS(uint16_t width)
  {
    _cfg.memory_width  = _cfg.panel_width  = width;
  }

  ESCPOS::ESCPOS(uint16_t width, uint16_t height) : ESCPOS(width)
  {
    _cfg.memory_height = _cfg.panel_height = height;
  }

  lgfx::color_depth_t ESCPOS::setColorDepth(lgfx::color_depth_t depth)
  {
    _write_depth = lgfx::color_depth_t::rgb565_2Byte;
    _read_depth = lgfx::color_depth_t::rgb565_2Byte;
    return lgfx::color_depth_t::rgb565_2Byte;
  }

  size_t ESCPOS::_get_buffer_length(void) const
  {
    return ((_cfg.panel_width + 7) & ~7) * _cfg.panel_height >> 3;
  }

  bool ESCPOS::init(bool use_reset)
  {
    if(_cfg.pin_busy >= 0)
      pinMode(_cfg.pin_busy, lgfx::pin_mode_t::input);

    if (!Panel_HasBuffer::init(use_reset))
    {
      return false;
    }

    if(use_reset)
    {
      uint8_t buf[] = {0x1B, 0x40};
      writeBytes(buf, sizeof(buf));
    }

    setInvert(_invert);
    setRotation(_rotation);
    setAutoDisplay(false);

    return true;
  }

  void ESCPOS::waitDisplay(void)
  {
    _wait_busy();
  }

  bool ESCPOS::displayBusy(void)
  {
    return _cfg.pin_busy >= 0 && lgfx::gpio_in(_cfg.pin_busy);
  }


  void ESCPOS::setInvert(bool invert)
  {
    _invert = invert;
  }

  void ESCPOS::writeBytes(const uint8_t* data, uint32_t length) {
    _wait_busy();
    _bus->writeBytes(data, length, true, true);
  }
  
  void ESCPOS::writeFillRectPreclipped(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, uint32_t rawcolor)
  {
    uint_fast16_t xs = x, xe = x + w - 1;
    uint_fast16_t ys = y, ye = y + h - 1;
    _xs = xs;
    _ys = ys;
    _xe = xe;
    _ye = ye;
//  _update_transferred_rect(xs, ys, xe, ye);

    lgfx::swap565_t color;
    color.raw = rawcolor;
    uint32_t value = (color.R8() + (color.G8() << 1) + color.B8()) >> 2;

    y = ys;
    do
    {
      x = xs;
      uint32_t idx = ((_cfg.panel_width + 7) & ~7) * y + x;
      auto btbl = &Bayer[(y & 3) << 2];
      do
      {
        bool flg = 256 <= value + btbl[x & 3];
        if (flg) _buf[idx >> 3] |=   0x80 >> (idx & 7);
        else     _buf[idx >> 3] &= ~(0x80 >> (idx & 7));
        ++idx;
      } while (++x <= xe);
    } while (++y <= ye);
  }

  void ESCPOS::writeImage(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, lgfx::pixelcopy_t* param, bool use_dma)
  {
    uint_fast16_t xs = x, xe = x + w - 1;
    uint_fast16_t ys = y, ye = y + h - 1;
//  _update_transferred_rect(xs, ys, xe, ye);

    auto readbuf = (lgfx::swap565_t*)alloca(w * sizeof(lgfx::swap565_t));
    auto sx = param->src_x32;
    h += y;
    do
    {
      uint32_t prev_pos = 0, new_pos = 0;
      do
      {
        new_pos = param->fp_copy(readbuf, prev_pos, w, param);
        if (new_pos != prev_pos)
        {
          do
          {
            auto color = readbuf[prev_pos];
            _draw_pixel(x + prev_pos, y, (color.R8() + (color.G8() << 1) + color.B8()) >> 2);
          } while (new_pos != ++prev_pos);
        }
      } while (w != new_pos && w != (prev_pos = param->fp_skip(new_pos, w, param)));
      param->src_x32 = sx;
      param->src_y++;
    } while (++y < h);
  }

  void ESCPOS::writePixels(lgfx::pixelcopy_t* param, uint32_t length, bool use_dma)
  {
    {
      uint_fast16_t xs = _xs;
      uint_fast16_t xe = _xe;
      uint_fast16_t ys = _ys;
      uint_fast16_t ye = _ye;
//    _update_transferred_rect(xs, ys, xe, ye);
    }
    uint_fast16_t xs   = _xs  ;
    uint_fast16_t ys   = _ys  ;
    uint_fast16_t xe   = _xe  ;
    uint_fast16_t ye   = _ye  ;
    uint_fast16_t xpos = _xpos;
    uint_fast16_t ypos = _ypos;

    static constexpr uint32_t buflen = 16;
    lgfx::swap565_t colors[buflen];
    int bufpos = buflen;
    do
    {
      if (bufpos == buflen) {
        param->fp_copy(colors, 0, std::min(length, buflen), param);
        bufpos = 0;
      }
      auto color = colors[bufpos++];
      _draw_pixel(xpos, ypos, (color.R8() + (color.G8() << 1) + color.B8()) >> 2);
      if (++xpos > xe)
      {
        xpos = xs;
        if (++ypos > ye)
        {
          ypos = ys;
        }
      }
    } while (--length);
    _xpos = xpos;
    _ypos = ypos;
  }

  void ESCPOS::readRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, void* dst, lgfx::pixelcopy_t* param)
  {
    auto readbuf = (lgfx::swap565_t*)alloca(w * sizeof(lgfx::swap565_t));
    param->src_data = readbuf;
    int32_t readpos = 0;
    h += y;
    do
    {
      uint32_t idx = 0;
      do
      {
        readbuf[idx] = _read_pixel(x + idx, y) ? ~0u : 0;
      } while (++idx != w);
      param->src_x32 = 0;
      readpos = param->fp_copy(dst, readpos, readpos + w, param);
    } while (++y < h);
  }

  bool ESCPOS::_wait_busy(uint32_t timeout)
  {
    if (displayBusy())
    {
      uint32_t start_time = millis();
      do
      {
        if (millis() - start_time > timeout) return false;
        delay(1);
      } while (displayBusy());
    }
    return true;
  }

  void ESCPOS::_draw_pixel(uint_fast16_t x, uint_fast16_t y, uint32_t value)
  {
    _rotate_pos(x, y);
    uint32_t idx = ((_cfg.panel_width + 7) & ~7) * y + x;
    bool flg = 256 <= value + Bayer[(x & 3) | (y & 3) << 2];
    if (flg) _buf[idx >> 3] |=   0x80 >> (idx & 7);
    else     _buf[idx >> 3] &= ~(0x80 >> (idx & 7));
  }

  bool ESCPOS::_read_pixel(uint_fast16_t x, uint_fast16_t y)
  {
    _rotate_pos(x, y);
    uint32_t idx = ((_cfg.panel_width + 7) & ~7) * y + x;
    return _buf[idx >> 3] & (0x80 >> (idx & 7));
  }

  void ESCPOS::print_ESC_asterisk(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h)
  {
    if(!w)
      w = _cfg.panel_width;
    
    if(!h)
      h = _cfg.panel_height;
    
    uint_fast16_t xs = x;
    uint_fast16_t ys = y;
    
    {
      uint8_t cmd_feed[] = {0x1B, 0x33, 24};
      writeBytes(cmd_feed, sizeof(cmd_feed));
    }
    
    uint8_t cmd[] = {0x1b, 0x2a, 33, (uint8_t)(w & 0xff), (uint8_t)((w >> 8) & 0xff)};
    uint8_t buf[3 * w];
    
    for(y = 0; y < h; y += 24) {
      memset(buf, _invert ? 0xff : 0x00, sizeof(buf));
      for(x = 0; x < w; x++) {
        for(uint16_t d = 0; d < 3; d++) {
          for(uint8_t b = 0; b < 8; b++)
            if(!_read_pixel(xs + x, ys + y + (d << 3) + b))
              buf[x * 3 + d] ^= 0x01 << (7 - b);
        }
      }
      writeBytes(cmd, sizeof(cmd));
      writeBytes(buf, sizeof(buf));
    }
  }

  void ESCPOS::print_GS_v(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h)
  {
    if(!w)
      w = _cfg.panel_width;
    
    if(!h)
      h = _cfg.panel_height;

    uint16_t bw = (w + 7) >> 3; // 8dots = 1bytes

    {
      // GS v 0 m xL xH yL yH d1 .... dk
      uint8_t cmd[] = {0x1d, 0x76, 0x30, 0x00, (uint8_t)(bw & 0xff), (uint8_t)((bw >> 8) & 0xff), (uint8_t)(h & 0xff), (uint8_t)((h >> 8) & 0xff)};
      writeBytes(cmd, sizeof(cmd));
    }
    
    uint_fast16_t xs = x;
    uint_fast16_t ys = y;
    
    uint8_t buf[bw];
    for(y = 0; y < h; y++) {
      memset(buf, _invert ? 0xff : 0x00, sizeof(buf));
      for(x = 0; x < bw; x++) {
        for(uint8_t b = 0; b < 8; b++)
          if(!_read_pixel(xs + (x << 3) + b, ys + y))
            buf[x] ^= 0x01 << (7 - b);
      }
      writeBytes(buf, sizeof(buf));
    }
  }
  
  void ESCPOS::print_GS_L(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h)
  {
    if(!w)
      w = _cfg.panel_width;
    
    if(!h)
      h = _cfg.panel_height;
    
    uint16_t bw = (w + 7) >> 3; // 8dots = 1bytes
    uint16_t plen = 10 + bw * h;
    
    {
      // GS ( L pL pH m fn a bx by c xL xH yL yH d1 .... dk
      uint8_t cmd1[] = {0x1d, 0x28, 0x4c, 4, 0, 48, 49, 51, 51
          , 0x1d, 0x28, 0x4c
          , (uint8_t)(plen & 0xff), (uint8_t)((plen >> 8) & 0xff)
        , 48, 112, 48, 1, 1, 49
        , (uint8_t)(w & 0xff), (uint8_t)((w >> 8) & 0xff)
        , (uint8_t)(h & 0xff), (uint8_t)((h >> 8) & 0xff)
      };
      writeBytes(cmd1, sizeof(cmd1));
    }
    
    uint_fast16_t xs = x;
    uint_fast16_t ys = y;
    
    uint8_t buf[bw];
    for(y = 0; y < h; y++) {
      memset(buf, _invert ? 0xff : 0x00, sizeof(buf));
      for(x = 0; x < w; x++) {
        for(uint8_t b = 0; b < 8; b++)
          if(!_read_pixel(x << 3 | b, y))
            buf[x] ^= 0x01 << (7 - b);
      }
      writeBytes(buf, sizeof(buf));
    }
    
    {
      uint8_t cmd2[] = {0x1d, 0x28, 0x4c, 2, 0, 48, 50};
      writeBytes(cmd2, sizeof(cmd2));
    }
  }
//----------------------------------------------------------------------------
 }
}
