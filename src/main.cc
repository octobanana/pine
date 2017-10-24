#include "util/parg.hh"

#include "util/log.hh"
#define LOG_MAIN "./log/main.log"

#include "util/color.hh"
namespace cl = OB::Color;

#include "util/string.hh"
namespace String = OB::String;

#include "util/termsize.hh"
namespace Term = OB::Termsize;

#include "util/pstream.hh"
using Process = OB::Process;

// #include "http/client/root_certificates.hpp"
// #include "http/client/client.hh"

#include <fmt/format.h>
using namespace fmt::literals;

#include <cmath>
#include <chrono>
#include <thread>
#include <stack>
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
using namespace std::string_literals;

int program_options(Parg& pg);

int program_options(Parg& pg)
{
  pg.name("app").version("0.1.0 (00.00.2017)");
  pg.usage("[flags] [options] [--] [arguments]");
  pg.description("app - a command line app");
  pg.info("Exit Codes:\n  0 -> normal\n  1 -> error\n");
  pg.author("octobanana (Brett Robinson) <octo.banana93 (at) gmail.com>");

  pg.set("help,h", "print the help output");
  pg.set("version,v", "print the program version");
  pg.set("color,c", "print the output in color");
  pg.set("file,f", "", "file to read from");

  pg.set_pos();
  pg.set_stdin();

  int status {pg.parse()};
  if (status > 0 && pg.get_stdin().empty())
  {
    std::cout << pg.print_help() << "\n";
    std::cout << "Error: " << "expected arguments" << "\n";
    return 1;
  }
  if (status < 0)
  {
    std::cout << pg.print_help() << "\n";
    std::cout << "Error: " << "invalid option/argument" << "\n";
    return 1;
  }
  if (pg.get<bool>("help"))
  {
    std::cout << pg.print_help();
    return 0;
  }
  if (pg.get<bool>("version"))
  {
    std::cout << pg.print_version();
    return 0;
  }
  return 0;
}

class regex_orderable : public std::regex
{
  public:
    regex_orderable(const char *regex_cstr) : std::regex(regex_cstr), str(regex_cstr) {}
    regex_orderable(std::string regex_str) : std::regex(regex_str), str(std::move(regex_str)) {}

    bool operator<(const regex_orderable &rhs) const noexcept
    {
      return str < rhs.str;
    }

    std::string to_string() const
    {
      return str;
    }

    friend std::ostream& operator<<(std::ostream&, const regex_orderable&);

  private:
    std::string str;
};

struct Debug
{
  bool all {false};
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

int main(int argc, char *argv[])
{
  OB::Log::clear(LOG_MAIN);
  OB::Log(OB::Log::INFO, LOG_MAIN) << "Starting Program";

  Parg pg {argc, argv};
  program_options(pg);

  // debug
  fmt::print("{}\n", cl::wrap(String::repeat("-", Term::width()), {cl::fg_magenta}));
  fmt::print("file: {}\n", pg.get("file"));
  fmt::print("{}\n", cl::wrap(String::repeat("-", Term::width()), {cl::fg_magenta}));

  if (! pg.find("file"))
  {
    // error
    fmt::print("Error: {}\n", "no file found");
    return 1;
  }

  std::ifstream ifile {pg.get("file")};
  if (! ifile.is_open())
  {
    // error
    fmt::print("Error: {}\n", "could not open file");
    return 1;
  }

  Flags flg;
  std::vector<Instruction> stk;
  std::vector<Instruction> reg;
  std::vector<int> cst;
  std::map<std::string, Label> lbl;
  std::map<std::string, Instruction> smap;

  auto const print_error = [](int line_num, std::string input, std::string msg)
  {
    fmt::print("Error: {}\n  [{}]: {}\n", msg, line_num, input);
  };

  auto const ins_mov = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 4)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // determine if key is reg or map
    std::string const key {m[2]};
    // bool rkey {false};
    // int ireg {0};
    // if (key.at(0) == '[' && key.at(key.size() - 1) == ']')
    // {
    //   std::string const k {key.substr(1, key.size() - 2)};
    //   ireg = std::stoi(k);
    //   rkey = true;
    // }

    std::string const val {m[3]};

    // determine type
    if (val.at(0) == '\'' && val.at(val.size() - 1) == '\'')
    {
      // string
      // if (rkey)
      // {
      //   reg[ireg].value = val.substr(1, val.size() - 2);
      //   reg[ireg].type = "str";
      //   reg[ireg].key = key;
      // }
      // else
      // {
        smap[key].value = val.substr(1, val.size() - 2);
        smap[key].type = "str";
        smap[key].key = key;
      // }
    }
    // else if (val.at(0) == '[' && val.at(val.size() - 1) == ']')
    // {
    //   // register
    //   std::string const k {val.substr(1, val.size() - 2)};
    //   int i = std::stoi(k);
    //   if (rkey)
    //   {
    //     reg[ireg] = reg.at(i);
    //   }
    //   else
    //   {
    //     smap[key] = reg.at(ireg);
    //   }
    // }
    else if (val.at(val.size() - 1) == 'f')
    {
      // double
      // if (rkey)
      // {
      //   reg[ireg].value = val.substr(0, val.size() - 1);
      //   reg[ireg].type = "dbl";
      //   reg[ireg].key = key;
      // }
      // else
      // {
        smap[key].value = val.substr(0, val.size() - 1);
        smap[key].type = "dbl";
        smap[key].key = key;
      // }
    }
    else
    {
      // int
      // if (rkey)
      // {
      //   reg[ireg].value = val;
      //   reg[ireg].type = "int";
      //   reg[ireg].key = key;
      // }
      // else
      // {
        smap[key].value = val;
        smap[key].type = "int";
        smap[key].key = key;
      // }
    }

    return 0;
  };

  auto const ins_add = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 4)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if keys exist
    if (smap.find(m[2]) == smap.end() || smap.find(m[3]) == smap.end())
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    auto& v1 = smap[m[2]];
    auto& v2 = smap[m[3]];

    // determine type
    if (v1.type == "int" && v2.type == "int")
    {
      // int
      v1.value = std::to_string(std::stoi(v1.value) + std::stoi(v2.value));
    }
    else if (v1.type == "dbl" && v2.type == "dbl")
    {
      // double
      v1.value = std::to_string(std::stod(v1.value) + std::stod(v2.value));
    }
    else
    {
      // string
      if (v1.type == "dbl")
      {
        v1.value = fmt::format("{:.1f}", std::stod(v1.value)) + v2.value;
      }
      else if (v2.type == "dbl")
      {
        v1.value = v1.value + fmt::format("{:.1f}", std::stod(v2.value));
      }
      else
      {
        v1.value = v1.value + v2.value;
      }
    }

    return 0;
  };

  auto const ins_sub = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 4)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if keys exist
    if (smap.find(m[2]) == smap.end() || smap.find(m[3]) == smap.end())
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    auto& v1 = smap[m[2]];
    auto& v2 = smap[m[3]];

    // determine type
    if (v1.type == "int" && v2.type == "int")
    {
      // int
      v1.value = std::to_string(std::stoi(v1.value) - std::stoi(v2.value));
    }
    else if (v1.type == "dbl" && v2.type == "dbl")
    {
      // double
      v1.value = std::to_string(std::stod(v1.value) - std::stod(v2.value));
    }
    else
    {
      // error
      print_error(line_num, input, "can't apply subtraction on strings");
      return 1;
    }

    return 0;
  };

  auto const ins_multiply = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 4)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if keys exist
    if (smap.find(m[2]) == smap.end() || smap.find(m[3]) == smap.end())
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    auto& v1 = smap[m[2]];
    auto& v2 = smap[m[3]];

    // determine type
    if (v1.type == "int" && v2.type == "int")
    {
      // int
      v1.value = std::to_string(std::stoi(v1.value) * std::stoi(v2.value));
    }
    else if (v1.type == "dbl" && v2.type == "dbl")
    {
      // double
      v1.value = std::to_string(std::stod(v1.value) * std::stod(v2.value));
    }
    else
    {
      // error
      print_error(line_num, input, "can't apply multiplication on strings");
      return 1;
    }

    return 0;
  };

  auto const ins_divide = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 4)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if keys exist
    if (smap.find(m[2]) == smap.end() || smap.find(m[3]) == smap.end())
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    auto& v1 = smap[m[2]];
    auto& v2 = smap[m[3]];

    // determine type
    if (v1.type == "int" && v2.type == "int")
    {
      // int
      v1.value = std::to_string(std::stoi(v1.value) / std::stoi(v2.value));
    }
    else if (v1.type == "dbl" && v2.type == "dbl")
    {
      // double
      v1.value = std::to_string(std::stod(v1.value) / std::stod(v2.value));
    }
    else
    {
      // error
      print_error(line_num, input, "can't apply division on strings");
      return 1;
    }

    return 0;
  };

  auto const ins_modulo = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 4)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if keys exist
    if (smap.find(m[2]) == smap.end() || smap.find(m[3]) == smap.end())
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    auto& v1 = smap[m[2]];
    auto& v2 = smap[m[3]];

    // determine type
    if (v1.type == "int" && v2.type == "int")
    {
      // int
      v1.value = std::to_string(std::stoi(v1.value) % std::stoi(v2.value));
    }
    else if (v1.type == "dbl" && v2.type == "dbl")
    {
      // double
      v1.value = std::to_string(std::remainder(std::stod(v1.value), std::stod(v2.value)));
    }
    else
    {
      // error
      print_error(line_num, input, "can't apply modulo on strings");
      return 1;
    }

    return 0;
  };

  auto const ins_clear = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if key exists
    auto it = smap.find(m[2]);
    if (it == smap.end())
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    smap.erase(it);

    return 0;
  };

  auto const ins_pop = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if key exist
    if (smap.find(m[2]) == smap.end())
    {
      print_error(line_num, input, "key does not exist");
      return 1;
    }

    // check if stack is empty
    if (stk.empty())
    {
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "the stack is empty", line_num, input);
      return 1;
    }

    // pop value off stack into map
    smap[m[2]] = stk.back();
    stk.pop_back();

    return 0;
  };

  auto const ins_push = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if key exist
    if (smap.find(m[2]) == smap.end())
    {
      print_error(line_num, input, "key does not exist");
      return 1;
    }

    // push value onto stack
    stk.emplace_back(smap[m[2]]);

    return 0;
  };

  auto const ins_print = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if key exist
    if (smap.find(m[2]) == smap.end())
    {
      print_error(line_num, input, "key does not exist");
      return 1;
    }

    auto& v = smap[m[2]];

    // stdout
    if (v.type == "dbl")
    {
      fmt::print("{:.1f}\n", std::stod(v.value));
    }
    else
    {
      fmt::print("{}\n", v.value);
    }

    return 0;
  };

  auto const ins_ask = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if key exist
    if (smap.find(m[2]) == smap.end())
    {
      print_error(line_num, input, "key does not exist");
      return 1;
    }

    // stdin
    std::string v;
    std::cout << "> " << std::flush;
    std::getline(std::cin, v);
    smap[m[2]].value = v;

    return 0;
  };

  auto const ins_debug = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 4)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check key value
    std::string const val {m[3]};
    if (val != "on" && val != "off")
    {
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "value must be either 'on' or 'off'", line_num, input);
      return 1;
    }
    bool v {false};
    if (val == "on")
    {
      v = true;
    }

    // check flag value
    std::string const flag {m[2]};
    if (flag == "all")
    {
      flg.dbg.all = v;
    }
    else if (flag == "map")
    {
      flg.dbg.map = v;
    }
    else if (flag == "stk")
    {
      flg.dbg.stk = v;
    }
    else if (flag == "lbl")
    {
      flg.dbg.lbl = v;
    }
    else if (flag == "flg")
    {
      flg.dbg.flg = v;
    }
    else if (flag == "jmp")
    {
      flg.dbg.jmp = v;
    }
    else if (flag == "rgx")
    {
      flg.dbg.rgx = v;
    }
    else if (flag == "lne")
    {
      flg.dbg.lne = v;
    }
    else
    {
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "invalid/missing arguments", line_num, input);
      return 1;
    }

    return 0;
  };

  auto const ins_label = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    std::string const label = m[2];

    // check if label exists
    if (lbl.find(label) == lbl.end())
    {
      // label is new
      lbl[label].line = line_num;
    }
    else
    {
      // label has been seen before
      if (lbl[label].line != line_num)
      {
        // error
        print_error(line_num, input, "label has already been declared");
        return 1;
      }
    }

    return 0;
  };

  auto const ins_compare = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 4)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    auto& v1 = smap[m[2]];
    auto& v2 = smap[m[3]];

    // compare key values
    if (v1.type == "int" && v2.type == "int")
    {
      // int
      if (std::stoi(v1.value) > std::stoi(v2.value))
      {
        // greater then
        flg.cmp = 1;
      }
      else if (std::stoi(v1.value) < std::stoi(v2.value))
      {
        // less then
        flg.cmp = -1;
      }
      else
      {
        // equal
        flg.cmp = 0;
      }
    }
    else if (v1.type == "dbl" && v2.type == "dbl")
    {
      // double
      if (std::stod(v1.value) > std::stod(v2.value))
      {
        // greater then
        flg.cmp = 1;
      }
      else if (std::stod(v1.value) < std::stod(v2.value))
      {
        // less then
        flg.cmp = -1;
      }
      else
      {
        // equal
        flg.cmp = 0;
      }
    }
    else
    {
      // string
      if (v1.value > v2.value)
      {
        // greater then
        flg.cmp = 1;
      }
      else if (v1.value < v2.value)
      {
        // less then
        flg.cmp = -1;
      }
      else
      {
        // equal
        flg.cmp = 0;
      }
    }

    return 0;
  };

  auto const ins_exit = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if key exists
    if (smap.find(m[2]) == smap.end())
    {
      print_error(line_num, input, "key does not exist");
      return 1;
    }

    auto& v = smap[m[2]];

    if (v.type != "int")
    {
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "invalid/missing arguments", line_num, input);
      return 1;
    }

    // exit program
    // TODO return exit code -1 and handle exit after main loop
    exit(std::stoi(v.value));

    return 0;
  };

  auto const ins_jump_equal = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    if (flg.cmp == 0)
    {
      flg.jmp.lbl = m[2];
      flg.jmp.now = true;
    }

    return 0;
  };

  auto const ins_jump_not_equal = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    if (flg.cmp != 0)
    {
      flg.jmp.lbl = m[2];
      flg.jmp.now = true;
    }

    return 0;
  };

  auto const ins_jump_less_then = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    if (flg.cmp < 0)
    {
      flg.jmp.lbl = m[2];
      flg.jmp.now = true;
    }

    return 0;
  };

  auto const ins_jump_greater_then = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
      // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    if (flg.cmp > 0)
    {
      flg.jmp.lbl = m[2];
      flg.jmp.now = true;
    }

    return 0;
  };

  auto const ins_jump = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
     // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    flg.jmp.lbl = m[2];
    flg.jmp.now = true;

    return 0;
  };

  auto const ins_ifile = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 4)
    {
     // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if keys exists
    if (smap.find(m[2]) == smap.end() || smap.find(m[3]) == smap.end())
    {
      print_error(line_num, input, "key does not exist");
      return 1;
    }

    std::ifstream ifile {smap[m[3]].value};
    if (! ifile.is_open())
    {
      // error
       print_error(line_num, input, "could not open file");
       return 1;
    }

    smap[m[2]].type = "str";
    smap[m[2]].value.assign((std::istreambuf_iterator<char>(ifile)),
      (std::istreambuf_iterator<char>()));
    ifile.close();

    return 0;
  };

  auto const ins_ofile = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 4)
    {
     // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if keys exists
    if (smap.find(m[2]) == smap.end() || smap.find(m[3]) == smap.end())
    {
      print_error(line_num, input, "key does not exist");
      return 1;
    }

    std::ofstream ofile {smap[m[3]].value};
    if (! ofile.is_open())
    {
      // error
       print_error(line_num, input, "could not open file");
       return 1;
    }

    ofile << smap[m[2]].value;
    ofile.close();

    return 0;
  };

  auto const ins_sleep = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
     // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if key exists
    if (smap.find(m[2]) == smap.end())
    {
      print_error(line_num, input, "key does not exist");
      return 1;
    }

    auto const& v {smap[m[2]]};

    if (v.type != "int")
    {
      print_error(line_num, input, "value must be an integer");
      return 1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(v.value)));

    return 0;
  };

  auto const ins_system = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 4)
    {
     // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if keys exists
    if (smap.find(m[2]) == smap.end() || smap.find(m[3]) == smap.end())
    {
      print_error(line_num, input, "key does not exist");
      return 1;
    }

    auto& v {smap[m[2]]};
    auto const& k {smap[m[3]]};

    if (k.type != "str")
    {
      print_error(line_num, input, "value must be a string");
      return 1;
    }

    v.type = "str";
    Process p {k.value};
    if (p.ec != 0)
    {
      print_error(line_num, input, fmt::format("could not run command", p.status));
      return 1;
    }
    v.value = p.out;
    flg.cmp = p.status;

    return 0;
  };

  auto const ins_run = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 3)
    {
     // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    cst.emplace_back(line_num);

    flg.jmp.lbl = m[2];
    flg.jmp.now = true;

    return 0;
  };

  auto const ins_return = [&](int line_num, std::string input, std::smatch m)
  {
    if (m.size() != 2)
    {
     // error
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if stack is empty
    if (cst.empty())
    {
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "the call stack is empty", line_num, input);
      return 1;
    }

    flg.ret.lne = cst.back();
    flg.ret.now = true;

    cst.pop_back();

    return 0;
  };

  // auto const ins_net = [&](int line_num, std::string input, std::smatch m)
  // {
  //   if (m.size() != 6)
  //   {
  //    // error
  //     print_error(line_num, input, "invalid/missing arguments");
  //     return 1;
  //   }

  //   // impl

  //   // check if keys exists
  //   if (smap.find(m[2]) == smap.end() || smap.find(m[3]) == smap.end()
  //     || smap.find(m[4]) == smap.end() || smap.find(m[5]) == smap.end())
  //   {
  //     print_error(line_num, input, "key does not exist");
  //     return 1;
  //   }

  //   auto& result {smap[m[2]]};
  //   std::string const port {smap[m[3]].value};
  //   std::string const url {smap[m[4]].value};
  //   std::string const target {smap[m[5]].value};

  //   if (port == "80")
  //   {
  //     auto res = Belle::request(url, target);
  //     result.value = res.body;
  //     result.type = "str";
  //   }
  //   else if (port == "443")
  //   {
  //     auto res = Belle::srequest(url, target);
  //     result.value = res.body;
  //     result.type = "str";
  //   }
  //   else
  //   {
  //     print_error(line_num, input, "unsupported port");
  //     return 1;
  //   }

  //   return 0;
  // };

  // TODO add ins name and documentation to each entry
  // TODO count num times each ins is called
  std::map<regex_orderable, std::function<int(int, std::string, std::smatch)>> imap {
    {"^\\s*(mov)\\s+([0-9a-zA-z]+)\\s+(.+)$", ins_mov},
    {"^\\s*(clr)\\s+([0-9a-zA-z]+)$", ins_clear},

    {"^\\s*(add)\\s+([0-9a-zA-z]+)\\s+([0-9a-zA-Z]+)$", ins_add},
    {"^\\s*(sub)\\s+([0-9a-zA-z]+)\\s+([0-9a-zA-Z]+)$", ins_sub},
    {"^\\s*(mlt)\\s+([0-9a-zA-z]+)\\s+([0-9a-zA-Z]+)$", ins_multiply},
    {"^\\s*(div)\\s+([0-9a-zA-z]+)\\s+([0-9a-zA-Z]+)$", ins_divide},
    {"^\\s*(mod)\\s+([0-9a-zA-z]+)\\s+([0-9a-zA-Z]+)$", ins_modulo},

    {"^\\s*(lbl)\\s+([0-9a-zA-z]+)$", ins_label},

    {"^\\s*(cmp)\\s+([0-9a-zA-z]+)\\s+([0-9a-zA-Z]+)$", ins_compare},

    // TODO jge
    // TODO jle
    {"^\\s*(jmp)\\s+([0-9a-zA-z]+)$", ins_jump},
    {"^\\s*(jeq)\\s+([0-9a-zA-z]+)$", ins_jump_equal},
    {"^\\s*(jne)\\s+([0-9a-zA-z]+)$", ins_jump_not_equal},
    {"^\\s*(jlt)\\s+([0-9a-zA-z]+)$", ins_jump_less_then},
    {"^\\s*(jgt)\\s+([0-9a-zA-z]+)$", ins_jump_greater_then},

    {"^\\s*(pop)\\s+([0-9a-zA-z]+)$", ins_pop},
    {"^\\s*(psh)\\s+([0-9a-zA-z]+)$", ins_push},

    // TODO add stdout format options
    {"^\\s*(prt)\\s+([0-9a-zA-z]+)$", ins_print},
    {"^\\s*(ask)\\s+([0-9a-zA-z]+)$", ins_ask},

    {"^\\s*(ifl)\\s+([0-9a-zA-z]+)\\s+([0-9a-zA-z]+)$", ins_ifile},
    {"^\\s*(ofl)\\s+([0-9a-zA-z]+)\\s+([0-9a-zA-z]+)$", ins_ofile},

    // TODO exe plugin system
    {"^\\s*(sys)\\s+([0-9a-zA-z]+)\\s+([0-9a-zA-z]+)$", ins_system},

    {"^\\s*(run)\\s+([0-9a-zA-z]+)$", ins_run},
    {"^\\s*(ret)$", ins_return},

    // TODO net
    // {"^(net)\\s([0-9a-zA-Z]+)\\s([0-9a-zA-Z]+)\\s([0-9a-zA-z]+)\\s([0-9a-zA-z]+)$", ins_net},

    {"^\\s*(dbg)\\s+([a-z]+)\\s([a-z]+)$", ins_debug},
    {"^\\s*(slp)\\s+([0-9a-zA-Z]+)$", ins_sleep},
    {"^\\s*(ext)\\s+([0-9a-zA-Z]+)$", ins_exit},
  };

  int line_num {0};
  std::map<int, uint64_t> lines;
  std::string input;
  while(std::getline(ifile, input))
  {
    // inc line number
    ++line_num;

    // note fstream byte total
    lines[line_num] = ifile.tellg();

    // handle returns
    if (flg.ret.now)
    {
      // fmt::print("return: {}\n", flg.ret.lne);
      ifile.seekg(lines[flg.ret.lne]);
      line_num = flg.ret.lne;
      flg.ret.now = false;
      continue;
    }

    // handle jumps
    if (flg.jmp.now)
    {
      // debug
      if (flg.dbg.all || flg.dbg.jmp)
      {
        fmt::print("jump: {}\n", flg.jmp.lbl);
      }

      // check if label exists
      if (lbl.find(flg.jmp.lbl) != lbl.end())
      {
        // seen label before
        ifile.seekg(lines[lbl.at(flg.jmp.lbl).line]);
        line_num = lbl.at(flg.jmp.lbl).line;
        flg.jmp.now = false;

        // debug
        if (flg.dbg.all || flg.dbg.jmp)
        {
          fmt::print("jump found: {}\n", flg.jmp.lbl);
        }

        // continue if found before
        continue;
      }
      else
      {
        // label hasn't been seen yet
        // debug
        if (flg.dbg.all || flg.dbg.jmp)
        {
          fmt::print("finding label: {}\n", flg.jmp.lbl);
        }
      }
    }

    // debug
    if (flg.dbg.all || flg.dbg.lne)
    {
      fmt::print("\n{}\n", cl::wrap(String::repeat("-", Term::width()), {cl::fg_green}));
      fmt::print("{}: {}\n", line_num, input);
      fmt::print("{}\n", cl::wrap(String::repeat("-", Term::width()), {cl::fg_green}));
    }

    // handle empty line
    if (input.empty())
    {
      // debug
      if (flg.dbg.all || flg.dbg.rgx)
      {
        fmt::print("empty: [{}]: {}\n", line_num, input);
      }

      continue;
    }

    // handle comment
    if (input.at(0) == '#')
    {
      // debug
      if (flg.dbg.all || flg.dbg.rgx)
      {
        fmt::print("comment: [{}]: {}\n", line_num, input);
      }

      continue;
    }

    // handle instruction
    bool valid {false};
    std::smatch match;
    for (auto& e : imap)
    {
      if (std::regex_match(input, match, e.first))
      {
        // debug
        if (flg.dbg.all || flg.dbg.rgx)
        {
          fmt::print("regex:\n");
          for (auto i = 0; i < match.size(); ++i)
          {
            std::string const s {match[i]};
            fmt::print("  [{}]: {}\n", i, s);
          }
          fmt::print("match: [{}]: {}\n", line_num, input);
          fmt::print("ins: {}\n", std::string(match[1]));
        }

        // handle jumps
        if (flg.jmp.now)
        {
          // debug
          if (flg.dbg.all || flg.dbg.jmp)
          {
            fmt::print("jump search: {}\n", flg.jmp.lbl);
          }
          if ("lbl" == match[1])
          {
            // debug
            if (flg.dbg.all || flg.dbg.jmp)
            {
              fmt::print("jump found label\n");
            }
            auto& ifunc = e.second;
            int status = ifunc(line_num, input, match);
            if (status == 0)
            {
              valid = true;
              if (flg.jmp.lbl == match[2])
              {
                flg.jmp.now = false;

                // debug
                if (flg.dbg.all || flg.dbg.jmp)
                {
                  fmt::print("jump found: {}\n", flg.jmp.lbl);
                }
              }
            }
            else
            {
              valid = false;
            }
            break;
          }
          else
          {
            valid = true;
            break;
          }
        }
        else
        {
          auto& ifunc = e.second;
          int status = ifunc(line_num, input, match);
          if (status == 0)
          {
            valid = true;
          }
          else
          {
            valid = false;
          }
          break;
        }
      }
    }

    // handle invalid instruction
    if (! valid)
    {
      // TODO should invalid ins break the program
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "invalid instruction", line_num, input);
      return 1;
    }

    // debug
    if (flg.dbg.all || flg.dbg.map)
    {
      fmt::print("map:\n");
      for (auto const& e : smap)
      {
        fmt::print("  {}\n", e.first);
        fmt::print("    key  -> {}\n", e.second.key);
        fmt::print("    val  -> {}\n", e.second.value);
        fmt::print("    type -> {}\n", e.second.type);
      }
    }
    if (flg.dbg.all || flg.dbg.lbl)
    {
      fmt::print("labels:\n");
      for (auto const& e : lbl)
      {
        fmt::print("  {} -> {}\n", e.first, e.second.line);
      }
    }
    if (flg.dbg.all || flg.dbg.stk)
    {
      fmt::print("stack:\n");
      for (auto const& e : stk)
      {
        fmt::print("  {}\n", e.key);
        fmt::print("    key  -> {}\n", e.key);
        fmt::print("    val  -> {}\n", e.value);
        fmt::print("    type -> {}\n", e.type);
      }
    }
    if (flg.dbg.all || flg.dbg.flg)
    {
      fmt::print("flags:\n");
      fmt::print("  cmp -> {}\n", flg.cmp);
      fmt::print("  dbg:\n");
      fmt::print("    all -> {}\n", flg.dbg.all);
      fmt::print("  jmp:\n");
      fmt::print("    now -> {}\n", flg.jmp.now);
      fmt::print("    lbl -> {}\n", flg.jmp.lbl);
    }

  }
  ifile.close();

  return 0;
}
