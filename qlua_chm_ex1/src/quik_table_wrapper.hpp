#pragma once

#include <map>
#include <memory>
#include <string>

#include <qluacpp/qlua>

struct QTable {
  // Создать и инициализировать экземпляр таблицы QTable
  QTable(const qlua::api& q);
    // удалить таблицу
  ~QTable();
  // отобразить в терминале окно с созданной таблицей
  void Show();

  // если окно с таблицей закрыто, возвращает «true»
  bool IsClosed() const;

  // возвращает строку, содержащую заголовок таблицы
  std::string GetCaption() const;

  // Задать заголовок таблицы
  int SetCaption(const std::string& s);

  // Добавить описание столбца <name> типа <c_type> в таблицу
  // <ff> – функция форматирования данных для отображения
  void AddColumn(const std::string& name,
                  const int c_type,
                  const unsigned int width,
                  std::function<std::string(unsigned int)> ff);

  void AddColumn(const std::string& name,
                  const int c_type,
                  const unsigned int width);
  void Clear();
  int AddLine();
  int SetPosition(const int x, const int y, const int dx, const int dy);
  std::tuple<int, int, int, int> QTable::GetPosition();

  struct column_desc {
    int id;
    int c_type;
  };

  template <typename T>
  bool SetValue(const int row, const std::string& col_name, T data) {
    auto found_col = columns_.find(col_name);
    if (found_col != columns_.end()) {
      auto col_ind = found_col->second.id;
      auto ff = int_column_formatters_.find(col_name);
      if (ff != int_column_formatters_.end()) {
        q_.SetCell(t_id_, row, col_ind, ff->second(data).c_str(), data);
        return true;
      } else {
        q_.SetCell(t_id_, row, col_ind, std::to_string(data).c_str(), data);
        return true; 
      }
    } else {
      return false;
    }
  }

  template <typename T>
  T GetValue(const int row, const std::string& name) {
    auto found_col = columns_.find(col_name);
    if (found_col != columns_.end()) {
      auto col_ind = found_col->second.id;
      return q_.GetCell<T>(t_id_, row, col_ind);
    } else {
      return T{};
    }  
  }

  int t_id() {
    return t_id_;
  }
private:
  qlua::api q_;
  int t_id_{0};
  std::string caption_;
  bool created_{false};
  int curr_col_{0};
  std::map<std::string, column_desc> columns_;
  std::map<std::string, std::function<std::string(int)>> int_column_formatters_;
};
