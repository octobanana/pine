#include "util/parg.hh"

#include "util/log.hh"
#define LOG_MAIN "./log/main.log"

#include "util/color.hh"
namespace cl = OB::Color;

#include "util/string.hh"
namespace String = OB::String;

#include "util/termsize.hh"
namespace Term = OB::Termsize;

#include <fmt/format.h>
using namespace fmt::literals;

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
  // TODO add flags for showing just some vars
};

struct Jump
{
  bool now {false};
  std::string lbl;
};

struct Flags
{
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
  std::stack<std::string> stk; // TODO should this be a vector
  std::vector<std::string> reg;
  std::map<std::string, Label> lbl;
  std::map<std::string, Instruction> smap;

  auto const print_error = [](int line_num, std::string input, std::string msg)
  {
    fmt::print("Error: {}\n  [{}]: {}\n", msg, line_num, input);
  };

  auto const dbg_ins_name = [](std::string name)
  {
    fmt::print("\n{}\n", String::to_upper(name));
  };

  auto const ins_mov = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 4)
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    std::string const key {m[2]};
    std::string const val {m[3]};

    // determine type
    if (val.at(0) == '\'' && val.at(val.size() - 1) == '\'')
    {
      // string
      smap[key].value = val.substr(1, val.size() - 2);
      smap[key].type = "str";
    }
    else if (val.at(val.size() - 1) == '\'')
    {
      // double
      smap[key].value = val.substr(0, val.size() - 2);
      smap[key].type = "dbl";
    }
    else
    {
      smap[key].value = val;
      smap[key].type = "int";
    }

    return 0;
  };

  auto const ins_add = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 4)
    {
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
      v1.value = v1.value + v2.value;
    }

    return 0;
  };

  auto const ins_sub = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 4)
    {
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

  auto const ins_clear = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
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
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
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
    smap[m[2]].value = stk.top();
    stk.pop();

    return 0;
  };

  auto const ins_push = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
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
    stk.emplace(m[2]);

    return 0;
  };

  auto const ins_print = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
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

    // stdout
    fmt::print("{}\n", smap[m[2]].value);

    return 0;
  };

  auto const ins_ask = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
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
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check flag value
    std::string const flag = m[2];
    if (flag == "all")
    {
      flg.dbg.all = true;
    }
    else if (flag == "off")
    {
      flg.dbg.all = false;
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
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    std::string const label = m[2];

    // check if key exist
    if (smap.find(label) == smap.end())
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
        print_error(line_num, input, "invalid/missing arguments");
        return 1;
      }
    }

    return 0;
  };

  auto const ins_compare = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 4)
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    auto& v1 = smap[m[2]];
    auto& v2 = smap[m[3]];

    // compare key values
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

    return 0;
  };

  auto const ins_exit = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
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
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if label exists
    if (smap.find(m[2]) == smap.end())
    {
      print_error(line_num, input, "label does not exist");
      return 1;
    }

    if (flg.cmp == 0)
    {
      flg.jmp.lbl = m[2];
      flg.jmp.now = true;
    }

    return 0;
  };

  auto const ins_jump_not_equal = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if label exists
    if (smap.find(m[2]) == smap.end())
    {
      print_error(line_num, input, "label does not exist");
      return 1;
    }

    if (flg.cmp != 0)
    {
      flg.jmp.lbl = m[2];
      flg.jmp.now = true;
    }

    return 0;
  };

  auto const ins_jump_less_then = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if label exists
    if (smap.find(m[2]) == smap.end())
    {
      print_error(line_num, input, "label does not exist");
      return 1;
    }

    if (flg.cmp < 0)
    {
      flg.jmp.lbl = m[2];
      flg.jmp.now = true;
    }

    return 0;
  };

  auto const ins_jump_greater_then = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if label exists
    if (smap.find(m[2]) == smap.end())
    {
      print_error(line_num, input, "label does not exist");
      return 1;
    }

    if (flg.cmp > 0)
    {
      flg.jmp.lbl = m[2];
      flg.jmp.now = true;
    }

    return 0;
  };

  auto const ins_jump = [&](int line_num, std::string input, std::smatch m)
  {
    // debug
    if (flg.dbg.all)
    {
      dbg_ins_name(m[1]);
    }

    // error
    if (m.size() != 3)
    {
      print_error(line_num, input, "invalid/missing arguments");
      return 1;
    }

    // impl

    // check if label exists
    if (smap.find(m[2]) == smap.end())
    {
      print_error(line_num, input, "label does not exist");
      return 1;
    }

    flg.jmp.lbl = m[2];
    flg.jmp.now = true;

    return 0;
  };

  std::map<regex_orderable, std::function<int(int, std::string, std::smatch)>> imap {
    {"^(mov)\\s([0-9a-zA-z]+)\\s([\"',.?! 0-9a-zA-Z]+)$", ins_mov},
    {"^(clr)\\s([0-9a-zA-z]+)$", ins_clear},

    {"^(add)\\s([0-9a-zA-z]+)\\s([0-9a-zA-Z]+)$", ins_add},
    {"^(sub)\\s([0-9a-zA-z]+)\\s([0-9a-zA-Z]+)$", ins_sub},

    {"^(lbl)\\s([0-9a-zA-z]+)$", ins_label},
    {"^(cmp)\\s([0-9a-zA-z]+)\\s([0-9a-zA-Z]+)$", ins_compare},
    {"^(jmp)\\s([0-9a-zA-z]+)$", ins_jump},
    {"^(jeq)\\s([0-9a-zA-z]+)$", ins_jump_equal},
    {"^(jne)\\s([0-9a-zA-z]+)$", ins_jump_not_equal},
    {"^(jlt)\\s([0-9a-zA-z]+)$", ins_jump_less_then},
    {"^(jgt)\\s([0-9a-zA-z]+)$", ins_jump_greater_then},

    {"^(pop)\\s([0-9a-zA-z]+)$", ins_pop},
    {"^(psh)\\s([0-9a-zA-z]+)$", ins_push},

    {"^(prt)\\s([0-9a-zA-z]+)$", ins_print},
    {"^(ask)\\s([0-9a-zA-z]+)$", ins_ask},

    {"^(dbg)\\s([a-z]+)$", ins_debug},

    {"^(ext)\\s([0-9a-zA-Z]+)$", ins_exit},
  };

  int line_num {0};
  std::vector<uint64_t> line_bytes;
  std::string input;
  while(std::getline(ifile, input))
  {
    // inc line number
    ++line_num;

    // note fstream byte total
    line_bytes.emplace_back(ifile.tellg());

    // handle jumps
    if (flg.jmp.now)
    {
      // debug
      if (flg.dbg.all)
      {
        fmt::print("jump: {}\n", flg.jmp.lbl);
      }

      // check if label exists
      auto it = lbl.find(flg.jmp.lbl);
      if (it != lbl.end())
      {
        // seen label before
        ifile.seekg(line_bytes.at(it->second.line - 1));
        line_num = it->second.line;
        flg.jmp.now = false;

        // debug
        if (flg.dbg.all)
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
        if (flg.dbg.all)
        {
          fmt::print("finding label: {}\n", flg.jmp.lbl);
        }
      }
    }

    // debug
    if (flg.dbg.all)
    {
      fmt::print("\n{}\n", cl::wrap(String::repeat("-", Term::width()), {cl::fg_green}));
      fmt::print("{}: {}\n", line_num, input);
      fmt::print("{}\n", cl::wrap(String::repeat("-", Term::width()), {cl::fg_green}));
    }

    // handle empty line
    if (input.empty())
    {
      // debug
      if (flg.dbg.all)
      {
        fmt::print("empty: [{}]: {}\n", line_num, input);
      }

      continue;
    }

    // handle comment
    if (input.at(0) == '#')
    {
      // debug
      if (flg.dbg.all)
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
        if (flg.dbg.all)
        {
          fmt::print("regex:\n");
          for (auto i = 0; i < match.size(); ++i)
          {
            std::string const s {match[i]};
            fmt::print("  [{}]: {}\n", i, s);
          }
        }

        // debug
        if (flg.dbg.all)
        {
          fmt::print("match: [{}]: {}\n", line_num, input);
          fmt::print("ins: {}\n", std::string(match[1]));
        }

        // handle jumps
        if (flg.jmp.now)
        {
          // debug
          if (flg.dbg.all)
          {
            fmt::print("jump search: {}\n", flg.jmp.lbl);
          }
          if ("lbl" == match[1])
          {
            // debug
            if (flg.dbg.all)
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
                if (flg.dbg.all)
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

    // print out debug info
    // debug
    if (flg.dbg.all)
    {
      fmt::print("map:\n");
      for (auto const& e : smap)
      {
        fmt::print("  {} -> {}\n", e.first, e.second.value);
      }
      fmt::print("labels:\n");
      for (auto const& e : lbl)
      {
        fmt::print("  {} -> {}\n", e.first, e.second.line);
      }
      fmt::print("stack:\n");
      auto stk_copy = stk;
      while (stk_copy.size() > 0)
      {
        fmt::print("  {}\n", stk_copy.top());
        stk_copy.pop();
      }
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
