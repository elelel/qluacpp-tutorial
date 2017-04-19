#include "order_db.hpp"

#include "json.hpp"

using json = nlohmann::json;

order_db::order_db(const std::string& filename) :
  filename_(filename) {
}
