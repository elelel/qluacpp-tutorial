#pragma once

#include <atomic>

#include "model.hpp"

struct paintable {
  paintable(const model& m,
            const int x,
            const int y,
            const int width,
            const int height,
            const int spacing,
            const int candle_width);
  
protected:
  int x_;
  int y_;
  int width_;
  int height_;
  int spacing_;
  int candle_width_;
  
  const model& m_;

  inline int abs_x(int rel_x) const {
    return x_ + rel_x;
  }

  inline int abs_y(int rel_y) const {
    return y_ + rel_y;
  }
  
};

struct chart_area : paintable {
  using paintable::paintable;

  void paint(void* paintstruct) const;

private:
  void paint_candles(void* paintstruct) const;
};

struct chart_bottom : paintable {
  using paintable::paintable;

  void paint(void* paintstruct) const;
};

struct gui {
  static gui& instance();
  void create(const std::string& title, std::shared_ptr<model> m);
  void repaint();
  void terminate();

  std::vector<std::string>& gui::data_unavailable_text();
  
  std::shared_ptr<model> model_;

  inline int chart_area_width() const;
  inline int chart_area_height() const;
  
  std::string title_;
  std::string wnd_class_{"qluacpp_draw_candles_rt"};
  void* hwnd_{nullptr};
  int wnd_height_{500};
  int wnd_width_{900};
  std::atomic<bool> done_{false};

  std::unique_ptr<chart_area> chart;
  std::unique_ptr<chart_bottom> bottom;
private:
  gui() {};

  std::vector<std::string> data_unavailable_text_;
};

