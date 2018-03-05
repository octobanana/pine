#include "parg.hh"
using Parg = OB::Parg;

#include "pine.hh"
using Pine = OB::Pine;

#include <string>
#include <iostream>

int program_options(Parg& pg);

int program_options(Parg& pg)
{
  pg.name("pine").version("0.2.0");
  pg.description("the pine language interpreter");
  pg.usage("[flags] [options] [--] [arguments]");
  pg.usage("[-v|--version]");
  pg.usage("[-h|--help]");
  pg.info("Exit Codes", {"0 -> normal", "1 -> error"});
  pg.author("octobanana (Brett Robinson) <octobanana.dev@gmail.com>");

  pg.set("help,h", "print the help output");
  pg.set("version,v", "print the program version");
  pg.set("file,f", "", "file_name", "file to read from");
  // pg.set("interactive,i", "start in interactive mode");

  // pg.set_pos();
  // pg.set_stdin();

  int status {pg.parse()};
  // uncomment if at least one argument is expected
  // if (status > 0 && pg.get_stdin().empty())
  // {
  //   std::cout << pg.print_help() << "\n";
  //   std::cout << "Error: " << "expected arguments" << "\n";
  //   return -1;
  // }
  if (status < 0)
  {
    std::cout << pg.print_help() << "\n";
    std::cout << "Error: " << pg.error() << "\n";
    return -1;
  }
  if (pg.get<bool>("help"))
  {
    std::cout << pg.print_help();
    return 1;
  }
  if (pg.get<bool>("version"))
  {
    std::cout << pg.print_version();
    return 1;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  Parg pg {argc, argv};
  int pstatus {program_options(pg)};
  if (pstatus > 0) return 0;
  else if (pstatus < 0) return 1;

  // if (pg.get<bool>("interpreter"))
  // {
  //   // start in interpreter mode
  //   // TODO handle interpreter mode
  //   return 0;
  // }

  if (! pg.find("file"))
  {
    // error
    std::cerr << "Error: missing file argument\n";
    return 1;
  }

  Pine pine;
  pine.set_file(pg.get("file"));
  pine.run();

  return 0;
}
