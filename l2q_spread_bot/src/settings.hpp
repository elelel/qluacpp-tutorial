#pragma once

struct settings_record {
  // Candidates refresh period
  size_t candidates_refresh_timeout{10};
  // Order size in lots size for single transaction
  size_t my_order_size{3};
  // What volume to ignore cumulatively when calculating spread in level 2 bid/ask quotes; in multiples of my_lot_size
  double vol_ignore_coeff{1.0};
  // Consider candidates only if the volume is greater than this number
  double min_volume{1.0};
  // Consider candidates only if spread ratio (1 - ask/bid) is greater than this number
  double min_spread{0.001};
  // Max number of instruments to consider as new candidates 
  size_t num_candidates{10};
  // New order speed limit
  size_t max_new_orders_per_hour{200};
};
