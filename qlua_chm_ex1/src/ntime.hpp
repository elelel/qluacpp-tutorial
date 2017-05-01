#pragma once

#include <ctime>
#include <string>
#include <vector>

std::string number_to_word(const int number) {
  // Simplified version, as we need it only for two-digit numbers
  const std::vector<std::string> words{"one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
  const std::vector<std::string> iwords{"ten", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
  const std::vector<std::string> twords = {"eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};

  if (number == 0) return "zero";
  if ((number >= 11) && (number <= 19)) return twords[number - 11];
  std::string ten_to_1;
  std::string ten_to_2;
  if ((number % 10) != 0) {
    ten_to_1 = words[(number % 10) - 1];
  } else {
    return iwords[number / 10 - 1];
  }
  if ((number / 10) != 0) {
    ten_to_2 = iwords[(number / 10) - 1];
  } else {
    return words[number % 10 - 1];
  }
  return ten_to_2 + "-" + ten_to_1;
}

std::string Time(const std::tm t) {
  auto hour = t.tm_hour % 12;
  auto minute = t.tm_min;
  std::string next_hour_word;
  if (hour == 0) {
    hour = 12;
    next_hour_word = "one ";
  } else {
    next_hour_word = number_to_word(hour + 1);
  }
  std::string hour_word = number_to_word(hour);
  if (minute == 0) return hour_word + " o'clock";
  if (minute == 30) return " half past " + hour_word;
  if (minute == 15) return " a quarter past " + hour_word;
  if (minute == 45) return " a quarter to " + next_hour_word;
  if (minute < 30) return number_to_word(minute) + " past " + hour_word;
  else return number_to_word(60 - minute) + " to " + next_hour_word;
}

std::string Seconds(const int s) {
  return number_to_word(s);
}

std::string NiceTime(const std::tm os_time) {
  std::string sec_str = " seconds";
  if (os_time.tm_sec == 1) sec_str = " second";
  return Time(os_time) +  " and " + Seconds(os_time.tm_sec) + sec_str;
}

