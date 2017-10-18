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

struct Instruction
{
  std::string name;
  std::string value;
  // std::string type;
};

int main(int argc, char *argv[])
{
  OB::Log::clear(LOG_MAIN);
  OB::Log(OB::Log::INFO, LOG_MAIN) << "Starting Program";

  Parg pg {argc, argv};
  program_options(pg);

  // debug
  fmt::print("{}\n", cl::wrap(String::repeat("-", Term::width()), {cl::fg_magenta}));

  // debug
  fmt::print("file: {}\n", pg.get("file"));

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

  std::vector<std::string> reg;
  std::stack<std::string> stk;
  std::map<std::string, Instruction> smap;

  auto const ins_mov = [&smap](int line_num, std::string input, std::smatch m)
  {
    // debug
    fmt::print("\nMOV\n");

    if (m.size() != 4)
    {
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "invalid/missing arguments", line_num, input);
      return 1;
    }

    smap[m[2]].name = m[2];

    std::string const v {m[3]};

    bool v_int = (v.find_first_not_of("0123456789") == std::string::npos);

    if (v_int)
    {
      // debug
      fmt::print("type: {}\n", "int");

      smap[m[2]].value = v;
      // smap[m[2]].type = "int";
    }
    else
    {
      // debug
      fmt::print("type: {}\n", "str");

      smap[m[2]].value = v.substr(1, v.size() - 2);
      // smap[m[2]].type = "string";
    }

    return 0;
  };

  auto const ins_add = [&smap, &stk](int line_num, std::string input, std::smatch m)
  {
    // debug
    fmt::print("\nADD\n");

    if (m.size() != 4)
    {
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "invalid/missing arguments", line_num, input);
      return 1;
    }

    auto v1 = smap[m[2]];
    auto v2 = smap[m[3]];

    bool v1_int = (v1.value.find_first_not_of("0123456789") == std::string::npos);
    bool v2_int = (v2.value.find_first_not_of("0123456789") == std::string::npos);

    if (v1_int && v2_int)
    {
      // debug
      fmt::print("type: {}\n", "int");

      int res = std::stoi(v1.value) + std::stoi(v2.value);
      stk.emplace(std::to_string(res));
      fmt::print("result: {}\n", stk.top());
    }
    else
    {
      // debug
      fmt::print("type: {}\n", "str");

      stk.emplace(v1.value + v2.value);
      fmt::print("result: {}\n", stk.top());
    }

    return 0;
  };

  auto const ins_sub = [&smap, &stk](int line_num, std::string input, std::smatch m)
  {
    // debug
    fmt::print("\nSUB\n");

    if (m.size() != 4)
    {
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "invalid/missing arguments", line_num, input);
      return 1;
    }

    auto v1 = smap[m[2]];
    auto v2 = smap[m[3]];

    bool v1_int = (v1.value.find_first_not_of("0123456789") == std::string::npos);
    bool v2_int = (v2.value.find_first_not_of("0123456789") == std::string::npos);

    if (v1_int && v2_int)
    {
      // debug
      fmt::print("type: {}\n", "int");

      int res = std::stoi(v1.value) - std::stoi(v2.value);
      stk.emplace(std::to_string(res));
      fmt::print("result: {}\n", stk.top());
    }
    else
    {
      fmt::print("Error: {}\n  [{}]: {}\n", "invalid/missing arguments", line_num, input);
      return 1;
    }

    return 0;
  };

  auto const ins_pop = [&smap, &stk](int line_num, std::string input, std::smatch m)
  {
    // debug
    fmt::print("\nPOP\n");

    if (m.size() != 3)
    {
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "invalid/missing arguments", line_num, input);
      return 1;
    }

    smap[m[2]].value = stk.top();
    stk.pop();

    return 0;
  };

  auto const ins_print = [&smap, &stk](int line_num, std::string input, std::smatch m)
  {
    // debug
    fmt::print("\nPRINT\n");

    if (m.size() != 3)
    {
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "invalid/missing arguments", line_num, input);
      return 1;
    }

    fmt::print("stdout: {}\n", smap[m[2]].value);

    return 0;
  };

  std::map<regex_orderable, std::function<int(int, std::string, std::smatch)>> imap {
    {"^(mov)\\s([0-9a-zA-z]+)\\s([\"',.?! 0-9a-zA-Z]+)$", ins_mov},
    {"^(add)\\s([0-9a-zA-z]+)\\s([0-9a-zA-Z]+)$", ins_add},
    {"^(sub)\\s([0-9a-zA-z]+)\\s([0-9a-zA-Z]+)$", ins_sub},
    {"^(pop)\\s([0-9a-zA-z]+)$", ins_pop},
    {"^(print)\\s([0-9a-zA-z]+)$", ins_print},
  };

  int line_num {0};
  std::string input;
  while(std::getline(ifile, input))
  {
    ++line_num;

    // debug
    fmt::print("\n{}: {}\n", line_num, input);
    fmt::print("map:\n");
    for (auto const& e : smap)
    {
      fmt::print("  {} -> {}\n", e.first, e.second.value);
    }

    if (input.empty())
    {
      // debug
      fmt::print("empty: [{}]: {}\n", line_num, input);

      continue;
    }

    if (input.at(0) == '#')
    {
      // debug
      fmt::print("comment: [{}]: {}\n", line_num, input);

      continue;
    }

    bool valid {false};
    std::smatch match;
    for (auto& e : imap)
    {
      if (std::regex_match(input, match, e.first))
      {
        // debug
        fmt::print("regex:\n");
        for (auto const& e : match)
        {
          std::string s {e};
          fmt::print("  val: {}\n", s);
        }

        // debug
        fmt::print("match: [{}]: {}\n", line_num, input);
        fmt::print("ins: {}\n", std::string(match[1]));

        auto& ifunc = e.second;
        ifunc(line_num, input, match);
        valid = true;
      }
    }

    if (! valid)
    {
      // error
      fmt::print("Error: {}\n  [{}]: {}\n", "invalid instruction", line_num, input);
      return 1;
    }

    fmt::print("map:\n");
    for (auto const& e : smap)
    {
      fmt::print("  {} -> {}\n", e.first, e.second.value);
    }
  }
  ifile.close();

  return 0;
}
