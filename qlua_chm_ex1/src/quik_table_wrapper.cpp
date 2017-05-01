#include "quik_table_wrapper.hpp"

// Создать и инициализировать экземпляр таблицы QTable
QTable::QTable(const qlua::api& q) :
  q_(q) {
  try {
    t_id_ = q_.AllocTable();    
  } catch (...) {
    q_.message("QTable: failed to create QTable", 3);
  }
}

// удалить таблицу
QTable::~QTable() {
  q_.DestroyTable(t_id_);
}

// отобразить в терминале окно с созданной таблицей
void QTable::Show() {
  if (q_.CreateWindow(t_id_) == 1) {
    if (caption_ != "") {
      // задать заголовок для окна
      q_.SetWindowCaption(t_id_, caption_.c_str());
    }
    created_ = true;    
  } else {
    q_.message(("Failed to create window for table " + std::to_string(t_id_)).c_str());
  }
}

// если окно с таблицей закрыто, возвращает «true»
bool QTable::IsClosed() const {
  return q_.IsWindowClosed(t_id_);
}

// возвращает строку, содержащую заголовок таблицы
std::string QTable::GetCaption() const {
  if (q_.IsWindowClosed(t_id_)) {
    return caption_;
  } else {
    return q_.GetWindowCaption(t_id_);
  }
}

// Задать заголовок таблицы
int QTable::SetCaption(const std::string& s) {
  int res{-1};
  caption_ = s;
  if (!q_.IsWindowClosed(t_id_)) {
    res = q_.SetWindowCaption(t_id_, s.c_str());
  }
  return res;
}


// Добавить описание столбца <name> типа <c_type> в таблицу
// <ff> – функция форматирования данных для отображения
void QTable::AddColumn(const std::string& name,
                        const int c_type,
                        const unsigned int width,
                        std::function<std::string(unsigned int)> ff) {
  AddColumn(name, c_type, width);
  if (c_type == q_.constant<int>("QTABLE_INT_TYPE")) {
    int_column_formatters_[name] = ff;
  }
}

void QTable::AddColumn(const std::string& name,
                        const int c_type,
                        const unsigned int width) {
  ++curr_col_;
  column_desc cd;
  cd.c_type = c_type;
  cd.id = curr_col_;
  q_.AddColumn(t_id_, curr_col_, name.c_str(), true, c_type, width);
  columns_.insert({name, cd});
}

void QTable::Clear() {
  q_.Clear(t_id_);
}

int QTable::AddLine() {
  return q_.InsertRow(t_id_, -1);
}

int QTable::SetPosition(const int x, const int y, const int dx, const int dy) {
  return q_.SetWindowPos(t_id_, x, y, dx, dy);
}

std::tuple<int, int, int, int> QTable::GetPosition() {
  return q_.GetWindowRect(t_id_);
}
