#ifndef OB_PINE_HH
#define OB_PINE_HH

#include <cmath>
#include <chrono>
#include <thread>
#include <limits>
#include <regex>
#include <map>
#include <functional>
#include <memory>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

namespace OB
{
class Regex : public std::regex
{
public:
  Regex(const char* cstr) : std::regex {cstr}, str_ {cstr} {}
  Regex(std::string str) : std::regex {str}, str_ {std::move(str)} {}

  bool operator<(const Regex &rhs) const noexcept
  {
    return str_ < rhs.str_;
  }

  std::string str() const
  {
    return str_;
  }

  friend std::ostream& operator<<(std::ostream& os, const Regex& obj);

private:
  std::string str_;
}; // class Regex

inline std::ostream& operator<<(std::ostream& os, const Regex& obj)
{
  os << obj.str_;
  return os;
}

class Pine
{
public:
  struct Debug
  {
    bool all {false};
    bool cmt {false};
    bool map {false};
    bool stk {false};
    bool lbl {false};
    bool flg {false};
    bool jmp {false};
    bool rgx {false};
    bool lne {false};
  };

  struct Return
  {
    bool now {false};
    int lne = 0;
  };

  struct Jump
  {
    bool now {false};
    std::string lbl;
  };

  struct Flags
  {
    Return ret;
    Jump jmp;
    Debug dbg;
    int cmp {0};
  };

  struct Label
  {
    int line {0};
  };

  struct Instruction
  {
    std::string key;
    std::string value;
    std::string type;
  };

  Pine();
  ~Pine();

  void set_file(std::string const _file);
  int run();

private:
  std::string file_main_;

  Flags flg;
  std::vector<Instruction> stk;
  std::vector<int> cst;
  std::map<std::string, Label> lbl;
  std::map<std::string, Instruction> smap;

};

} // namespace OB

#endif // OB_PINE_HH
