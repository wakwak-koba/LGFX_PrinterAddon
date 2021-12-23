#include "EM5820.hpp"
#include <lgfx/v1/Bus.hpp>
#include <lgfx/v1/platforms/common.hpp>
#include <lgfx/v1/misc/pixelcopy.hpp>
#include <lgfx/v1/misc/colortype.hpp>

#ifdef min
#undef min
#endif

namespace lgfx_addon
{
 inline namespace aiebcy
 {
//----------------------------------------------------------------------------

  static constexpr uint8_t Bayer[16] = { 8, 200, 40, 232, 72, 136, 104, 168, 56, 248, 24, 216, 120, 184, 88, 152 };

  EM5820::EM5820(void)
  {
    _cfg.memory_width  = _cfg.panel_width  = 384;
  }

  lgfx::color_depth_t EM5820::setColorDepth(lgfx::color_depth_t depth)
  {
    _write_depth = lgfx::color_depth_t::rgb565_2Byte;
    _read_depth = lgfx::color_depth_t::rgb565_2Byte;
    return lgfx::color_depth_t::rgb565_2Byte;
  }

  size_t EM5820::_get_buffer_length(void) const
  {
    return ((_cfg.panel_width + 7) & ~7) * _cfg.panel_height >> 3;
  }

  bool EM5820::init(bool use_reset)
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

  void EM5820::waitDisplay(void)
  {
    _wait_busy();
  }

  bool EM5820::displayBusy(void)
  {
    return _cfg.pin_busy >= 0 && lgfx::gpio_in(_cfg.pin_busy);
  }

  void EM5820::display(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h)
  {
    w = (_cfg.panel_width + 7) >> 3;    // 8dots = 1bytes
    h = _cfg.panel_height;
      uint8_t buf[] = {0x1d, 0x76, 0x30, 0x00, (uint8_t)(w & 0xff), (uint8_t)((w >> 8) & 0xff), (uint8_t)(h & 0xff), (uint8_t)((h >> 8) & 0xff)};
    writeBytes(buf, sizeof(buf));
    
    for(uint8_t y = 0; y < h; y++) {
      for(uint8_t x = 0; x < w; x++) {
        uint8_t dt = _invert ? 0xff : 0x00;
        for(uint8_t b = 0; b < 8; b++)
          if(!_read_pixel((x << 3) | b, y))
              dt ^= 0x01 << (7 - b);
        writeBytes(&dt, 1);
      }
    }
  }

  void EM5820::setInvert(bool invert)
  {
    _invert = invert;
  }

  void EM5820::writeBytes(const uint8_t* data, uint32_t length) {
    _wait_busy();
    _bus->writeBytes(data, length, true, true);
  }
  
  void EM5820::writeFillRectPreclipped(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, uint32_t rawcolor)
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

  void EM5820::writeImage(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, lgfx::pixelcopy_t* param, bool use_dma)
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

  void EM5820::writePixels(lgfx::pixelcopy_t* param, uint32_t length, bool use_dma)
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

  void EM5820::readRect(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, void* dst, lgfx::pixelcopy_t* param)
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

  bool EM5820::_wait_busy(uint32_t timeout)
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

  void EM5820::_draw_pixel(uint_fast16_t x, uint_fast16_t y, uint32_t value)
  {
    _rotate_pos(x, y);
    uint32_t idx = ((_cfg.panel_width + 7) & ~7) * y + x;
    bool flg = 256 <= value + Bayer[(x & 3) | (y & 3) << 2];
    if (flg) _buf[idx >> 3] |=   0x80 >> (idx & 7);
    else     _buf[idx >> 3] &= ~(0x80 >> (idx & 7));
  }

  bool EM5820::_read_pixel(uint_fast16_t x, uint_fast16_t y)
  {
    _rotate_pos(x, y);
    uint32_t idx = ((_cfg.panel_width + 7) & ~7) * y + x;
    return _buf[idx >> 3] & (0x80 >> (idx & 7));
  }

//----------------------------------------------------------------------------
 }
}
