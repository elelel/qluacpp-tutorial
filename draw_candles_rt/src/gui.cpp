#include "gui.hpp"

#include "windows.h"

#include <algorithm>
#include <cmath>
#include <thread>

paintable::paintable(const model& m,
                     const int x,
                     const int y,
                     const int width,
                     const int height,
                     const int spacing,
                     const int candle_width) :
  m_(m),
  x_(x),
  y_(y),
  width_(width),
  height_(height),
  spacing_(spacing),
  candle_width_(candle_width) {

  }

void chart_area::paint(void* paintstruct) const {
  //  paint_grid(paintstruct);
  paint_candles(paintstruct);
}

void chart_area::paint_candles(void* paintstruct) const {
  auto ps = (PAINTSTRUCT*)paintstruct;
  const auto& cs = m_.candles();
  if (cs.size() > 1) {
    auto max_price = m_.max_price();
    auto min_price = m_.min_price();
    auto price_z = max_price - min_price;
    auto max_volume = m_.max_volume();
    auto min_volume = m_.min_volume();
    auto volume_z = max_volume - min_volume;

    std::cout << "Close to open " << std::endl;
    // Close to open lines
    for (size_t i = 0; i < cs.size() - 1; ++i) {
      double l_close = (cs[i].close - min_price)/price_z;
      double r_open = (cs[i+1].open - min_price)/price_z;
      long left = (i * candle_width_) + candle_width_ - spacing_ - 1;
      long right = (i + 1) * candle_width_ + spacing_ + 1;
      std::vector<POINT> line_points{
        {abs_x(left), abs_y(height_ - std::lround(l_close * height_))},
          {abs_x(right), abs_y(height_ - std::lround(r_open * height_))}};
      Polyline(ps->hdc, line_points.data(), line_points.size());
    }

    std::cout << "Bodies " << std::endl;
    // Candle bodies
    size_t i{0};
    for (const auto& c : cs) {
      auto open = (c.open - min_price)/price_z;
      auto close = (c.close - min_price)/price_z;
      auto high = (c.high - min_price)/price_z;
      auto low = (c.low - min_price)/price_z;
      std::vector<double> open_close{open, close};
      std::sort(open_close.begin(), open_close.end());
      long left = i * candle_width_ + spacing_;
      long right = (i * candle_width_) + (candle_width_ - spacing_);
      long top = height_ - std::lround(open_close[1] * height_) - 1;
      long bottom = height_ - std::lround(open_close[0] * height_) + 1;

      long v_line_top = height_ - std::lround(high * height_);
      long v_line_bottom = height_ - std::lround(low * height_);
      long middle_x = i * candle_width_ + (candle_width_ / 2);
      
      std::vector<POINT> v_line{{abs_x(middle_x), abs_y(v_line_top)}, {abs_x(middle_x), abs_y(v_line_bottom)}};
      Polyline(ps->hdc, v_line.data(), v_line.size());

      auto vol = (c.volume - min_volume) / volume_z;
      BYTE red = BYTE(std::lround(vol * 255.0));
      BYTE blue = BYTE(std::lround((1.0 - vol) * 255.0));
      auto brush = CreateSolidBrush(RGB(red, 128, blue));
      SelectObject(ps->hdc, brush);
      RoundRect(ps->hdc, abs_x(left), abs_y(top), abs_x(right), abs_y(bottom), 3, 3);
      DeleteObject(brush);
      ++i;
    }
  }
}

void chart_bottom::paint(void* paintstruct) const {
  auto ps = (PAINTSTRUCT*)paintstruct;
  const auto& cs = m_.candles();
  if (cs.size() > 1) {
    // Candle dates
    const float factor = (2.0f * 3.1416f) / 360.0f;
    const float rot = 90.0f * factor;
    XFORM xfm = { 0.0 };
    xfm.eM11 = cos(rot);
    xfm.eM12 = sin(rot);
    xfm.eM21 = -sin(rot);
    xfm.eM22 = cos(rot);
    auto old_mode = GetGraphicsMode(ps->hdc);
    SetGraphicsMode(ps->hdc, GM_ADVANCED);
    SetWorldTransform(ps->hdc, &xfm);
    for (size_t i = 0; i < cs.size(); ++i) {
      // Write time for every 5 candles
      if (((i % 5) == 0) || (i == cs.size() - 1)) {
        TextOut(ps->hdc, abs_x(i * candle_width_), abs_y(5), cs[i].time.c_str(), cs[i].time.size());
      }
    }
    SetGraphicsMode(ps->hdc, old_mode);
      
    const long time_labels_height = 100;

    // Last price
    auto max_volume = m_.max_volume();
    auto min_volume = m_.min_volume();

    std::string last_str = std::to_string(cs[cs.size()-1].close);
    while ((last_str.find(".") != std::string::npos) && (last_str[last_str.size() - 1] == '0'))
      last_str = last_str.substr(0, last_str.size() - 1);
    TextOut(ps->hdc, abs_x(width_ - 80), abs_y(20), last_str.c_str(), last_str.size());

    // Volume gauge
    const long x_offset = 10;
    std::string vol_txt{"Volume:"};
    TextOut(ps->hdc, abs_x(x_offset), abs_y(0), vol_txt.c_str(), vol_txt.size());
    std::string min_vol_str = std::to_string(std::lround(min_volume));
    std::string max_vol_str = std::to_string(std::lround(max_volume));
    TextOut(ps->hdc, abs_x(x_offset - 3), abs_y(5 + 20), min_vol_str.c_str(), min_vol_str.size());
    TextOut(ps->hdc, abs_x(x_offset + 255 - 3), abs_y(5 + 20), max_vol_str.c_str(), max_vol_str.size());
    for (int i = 1; i < 255; ++i) {
      std::vector<POINT> line{{abs_x(x_offset + i), abs_y(50)},
          {abs_x(x_offset + i), abs_y(75)}};
      auto pen = CreatePen(PS_SOLID, 1, RGB(i, 128, 255-i));
      SelectObject(ps->hdc, pen);
      Polyline(ps->hdc, line.data(), line.size());
      DeleteObject(pen);
    }
    auto pen = CreatePen(PS_SOLID, 1, 0);
    SelectObject(ps->hdc, pen);
    SelectObject(ps->hdc, GetStockObject(NULL_BRUSH));
    Rectangle(ps->hdc, abs_x(x_offset),  abs_y(50), abs_x(x_offset + 255), abs_y(75));
    std::vector<POINT> vol_min_line{{abs_x(x_offset), abs_y(50 - 10)},
        {abs_x(x_offset), abs_y(50)}};
    std::vector<POINT> vol_max_line{{abs_x(x_offset + 254), abs_y(50 - 10)},
        {abs_x(x_offset + 254), abs_y(50)}};
    Polyline(ps->hdc, vol_min_line.data(), vol_min_line.size());
    Polyline(ps->hdc, vol_max_line.data(), vol_max_line.size());
    DeleteObject(pen);
  }
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  auto& g = gui::instance();
  
  switch (uMsg) { 
  case WM_CREATE:
    return 0;
  case WM_PAINT: {
    g.model_->wait();
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);

    const auto& cs = g.model_->candles();

    if (cs.size() > 1) {
      if (g.chart) g.chart->paint(&ps);
      if (g.bottom) g.bottom->paint(&ps);
    } else {
      long i{0};
      for (const std::string& s : g.data_unavailable_text()) {
        TextOut(ps.hdc, 10, 10 + i, s.c_str(), s.size());
        i += 18;
      }
    }
    EndPaint(hwnd, &ps);
    g.model_->notify();
    return 0;
  }
  case WM_SIZE:
    return 0;
  case WM_DESTROY:
    g.done_ = true;
    return 0;
  default:
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }
  return 0;
}

static std::thread message_thread;

void thread_proc() {
  auto hinstance = GetModuleHandle(NULL);
  auto& hwnd = gui::instance().hwnd_;
  hwnd = CreateWindowA(gui::instance().wnd_class_.c_str(),        // name of window class 
                       gui::instance().title_.c_str(),            // title-bar string 
                       WS_SYSMENU | WS_CAPTION, // top-level window 
                       CW_USEDEFAULT,       // default horizontal position 
                       CW_USEDEFAULT,       // default vertical position 
                       gui::instance().wnd_width_,       // default width 
                       gui::instance().wnd_height_,       // default height 
                       (HWND) NULL,         // no owner window 
                       (HMENU) NULL,        // use class menu 
                       hinstance,           // handle to application instance 
                       (LPVOID) NULL);      // no window-creation data
  if (!hwnd) 
    throw std::runtime_error("Could not create window");
  ShowWindow((HWND)hwnd, SW_SHOWNORMAL);
  UpdateWindow((HWND)hwnd);

  MSG msg;
  while (!gui::instance().done_) {
    auto rslt = GetMessage(&msg, (HWND)hwnd, 0, 0);
    switch (rslt) {
    case -1:
      return;
    case 0:
      return;
    default:
      TranslateMessage(&msg); 
      DispatchMessage(&msg); 
    }
  }
  hwnd = nullptr;
}

gui& gui::instance() {
  static gui g;
  return g;
}

std::vector<std::string>& gui::data_unavailable_text() {
  return data_unavailable_text_;
}

void gui::create(const std::string& title, std::shared_ptr<model> m) {
  if (m == nullptr) {
    throw std::runtime_error("Can't create candles window without model");
  }
  if (hwnd_ != nullptr) {
    PostMessage((HWND)hwnd_, WM_DESTROY, 0, 0);
  }
  hwnd_ = nullptr;
  done_ = false;
  title_ = title;
  model_ = m;
  auto hinstance = GetModuleHandle(NULL);
  chart = std::make_unique<chart_area>(*m,
                                       0, 0,
                                       wnd_width_ - 6, wnd_height_ /3 * 2,
                                       3,
                                       (wnd_width_ - 6) / m->max_count());
  bottom = std::make_unique<chart_bottom>(*m,
                                          0, wnd_height_ /3 * 2 + 1,
                                          wnd_width_ - 6, wnd_height_ /3,
                                          3,
                                          (wnd_width_ - 6) / m->max_count());

  WNDCLASS wc;
  if (!GetClassInfo(hinstance, wnd_class_.c_str(), &wc)) {
    // Register the main window class. 
    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.cbClsExtra = 0; 
    wc.cbWndExtra = 0; 
    wc.hInstance = hinstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); 
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); 
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = wnd_class_.c_str();
    wc.lpfnWndProc = wnd_proc;
 
    if (!RegisterClass(&wc))
      throw std::runtime_error("Could not register window class");
  }
  message_thread = std::move(std::thread(thread_proc));
  message_thread.detach();

  model_->wait();
  model_->on_new_data = [this] () {
    if (hwnd_ != nullptr) {
      InvalidateRect((HWND)hwnd_, NULL, true);
      PostMessage((HWND)hwnd_, WM_PAINT, 0, 0);
    }
  };
  model_->notify();
}

void gui::terminate() {
  if (hwnd_ != nullptr) {
    PostMessage((HWND)hwnd_, WM_DESTROY, 0, 0);
  }
}

int gui::chart_area_width() const {
  return wnd_width_ - 6;
}

int gui::chart_area_height() const {
  return wnd_height_ / 3 * 2;
}

