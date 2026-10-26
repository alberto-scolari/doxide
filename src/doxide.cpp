#include "Driver.hpp"
#include <generated/config.h>

#include <CLI/CLI.hpp>

int main(int argc, char** argv) {
  Driver driver;

  /* read config file first, so that command-line options can override */
  driver.config();

  /* handle command line */
  CLI::App app{"Modern documentation for modern C++.\n"};
  app.get_formatter()->column_width(30);
  app.add_option("--title",
      driver.title,
      "Main page title.");
  app.add_option("--description",
      driver.description,
      "Main page description.");
  app.add_option("--output", driver.output,
      "Output directory.");
  app.add_option("--coverage", driver.coverage,
      "Code coverage file (.gcov or .json).");
  app.add_option("--style", driver.style,
      "Specify the style of documentation: plain or mkdocs.");
  app.set_version_flag("--version,-v", PACKAGE_VERSION, "Doxide version.");
  app.add_subcommand("init",
      "Initialize configuration files.")->
      fallthrough()->
      callback([&]() { driver.init(); });
  app.add_subcommand("build",
      "Build documentation in output directory.")->
      fallthrough()->
      callback([&]() { driver.build(); });
  app.add_subcommand("watch",
      "Watch the documentation's source files and rebuild it on changes.")->
      fallthrough()->
      callback([&]() { driver.watch(); });
  app.add_subcommand("clean",
      "Clean output directory.")->
      fallthrough()->
      callback([&]() { driver.clean(); });
  app.add_subcommand("cover",
      "Output code coverage data to stdout in JSON format.")->
      fallthrough()->
      callback([&]() { driver.cover(); });
  app.require_subcommand(1);
  CLI11_PARSE(app, argc, argv);
}
