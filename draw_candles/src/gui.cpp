#include "gui.hpp"

#include "windows.h"

#include <algorithm>
#include <cmath>
#include <thread>

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) { 
  case WM_CREATE:
    return 0;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    const auto& d = gui::instance().model_->view();

    auto chart_height = gui::instance().chart_height_;
    auto candle_width = gui::instance().candle_width_;

    if (d.candles.size() < 2) {
      std::string msg("Not enough candles to draw the chart.");
      TextOut(ps.hdc, 10, chart_height / 2,  msg.c_str(), msg.size());
      msg = "Make sure that chart tag (identifier) '" + gui::instance().model_->tag() + "' exists:";
      TextOut(ps.hdc, 10, chart_height / 2 + 18,  msg.c_str(), msg.size());
      msg = "right-click on the candles in Quik chart and set tag to '" +
        gui::instance().model_->tag() + "' on the second tab.";
      TextOut(ps.hdc, 10, chart_height / 2 + 18 * 2,  msg.c_str(), msg.size());
      msg = "Then, close this window and start the script again";
      TextOut(ps.hdc, 10, chart_height / 2 + 18 * 3,  msg.c_str(), msg.size());
      return 0;
    }

    long spacing = 3;
    auto z = d.max_price - d.min_price;

    // Close to open lines
    std::vector<model::candle> candles_vec(d.candles.begin(), d.candles.end());
    for (size_t i = 0; i < candles_vec.size() - 1; ++i) {
      double l_close = (candles_vec[i].close - d.min_price)/z;
      double r_open = (candles_vec[i+1].open - d.min_price)/z;
      long left = (i * candle_width) + candle_width - spacing - 1;
      long right = (i + 1) * candle_width + spacing + 1;
      std::vector<POINT> line_points{{left, chart_height - std::lround(l_close * chart_height)},
          {right, chart_height - std::lround(r_open * chart_height)}};
      Polyline(ps.hdc, line_points.data(), line_points.size());
    }
    // Candle bodies
    size_t i{0};
    for (const auto& c : d.candles) {
      auto open = (c.open - d.min_price)/z;
      auto close = (c.close - d.min_price)/z;
      auto high = (c.high - d.min_price)/z;
      auto low = (c.low - d.min_price)/z;
      std::vector<double> open_close{open, close};
      std::sort(open_close.begin(), open_close.end());
      long left = i * candle_width + spacing;
      long right = (i * candle_width) + (candle_width - spacing);
      long top = chart_height - std::lround(open_close[1] * chart_height) - 1;
      long bottom = chart_height - std::lround(open_close[0] * chart_height) + 1;

      long v_line_top = chart_height - std::lround(high * chart_height);
      long v_line_bottom = chart_height - std::lround(low * chart_height);
      long middle_x = i * candle_width + (candle_width / 2);
      
      std::vector<POINT> v_line{{middle_x, v_line_top}, {middle_x, v_line_bottom}};
      Polyline(ps.hdc, v_line.data(), v_line.size());

      auto vol_z = d.max_volume - d.min_volume;
      auto vol = (c.volume - d.min_volume) / vol_z;
      BYTE red = BYTE(std::lround(vol * 255.0));
      BYTE blue = BYTE(std::lround((1.0 - vol) * 255.0));
      auto brush = CreateSolidBrush(RGB(red, 128, blue));
      SelectObject(ps.hdc, brush);
      RoundRect(ps.hdc, left, top, right, bottom, 3, 3);
      DeleteObject(brush);
      ++i;
    }
    // Voluem gauge
    const long offset = 10;
    std::string vol_txt{"Volume:"};
    TextOut(ps.hdc, offset, chart_height + 5, vol_txt.c_str(), vol_txt.size());
    std::string min_vol_str = std::to_string(std::lround(d.min_volume));
    std::string max_vol_str = std::to_string(std::lround(d.max_volume));
    TextOut(ps.hdc, offset - 3, chart_height + 5 + 20, min_vol_str.c_str(), min_vol_str.size());
    TextOut(ps.hdc, offset + 255 - 3, chart_height + 5 + 20, max_vol_str.c_str(), max_vol_str.size());
    for (int i = 1; i < 255; ++i) {
      std::vector<POINT> line{{offset + i, chart_height + 50},
        {offset + i, chart_height + 75}};
      auto pen = CreatePen(PS_SOLID, 1, RGB(i, 128, 255-i));
      SelectObject(ps.hdc, pen);
      Polyline(ps.hdc, line.data(), line.size());
      DeleteObject(pen);
    }
    auto pen = CreatePen(PS_SOLID, 1, 0);
    SelectObject(ps.hdc, pen);
    SelectObject(ps.hdc, GetStockObject(NULL_BRUSH));
    Rectangle(ps.hdc, offset, chart_height + 50, offset + 255, chart_height + 75);
    std::vector<POINT> vol_min_line{{offset, chart_height + 50 - 10},
        {offset, chart_height + 50}};
    std::vector<POINT> vol_max_line{{offset + 254, chart_height + 50 - 10},
        {offset + 254, chart_height + 50}};
    Polyline(ps.hdc, vol_min_line.data(), vol_min_line.size());
    Polyline(ps.hdc, vol_max_line.data(), vol_max_line.size());
    DeleteObject(pen);
    EndPaint(hwnd, &ps);
    return 0;
  }
  case WM_SIZE:
    return 0;
  case WM_DESTROY:
    gui::instance().done_ = true;
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
}

gui& gui::instance() {
  static gui g;
  return g;
}

void gui::repaint() {
  if (hwnd_ != nullptr)
    PostMessage((HWND)hwnd_, WM_PAINT, 0, 0);
}

void gui::create(const std::string& title, model* m) {
  if (hwnd_ != nullptr) {
    SendMessage((HWND)hwnd_, WM_DESTROY, 0, 0);
  }
  hwnd_ = nullptr;
  done_ = false;
  title_ = title;
  model_ = m;
  chart_width_ = wnd_width_;
  chart_height_ = wnd_height_ - 45 - 100;
  candle_width_ = chart_width_ / m->max_count();
  auto hinstance = GetModuleHandle(NULL);

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
}

void gui::terminate() {
  if (hwnd_ != nullptr) {
    PostMessage((HWND)hwnd_, WM_DESTROY, 0, 0);
  }
}

