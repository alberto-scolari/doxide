#include "Driver.hpp"

#include "DriverConfigParser.hpp"
#include <generated/config.h>

#include <CLI/CLI.hpp>

#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <string_view>

enum class CliCommand {
  NONE,
  INIT,
  BUILD,
  WATCH,
  CLEAN,
  COVER
};

static constexpr std::initializer_list<std::string_view> DOXIDE_CONF_FILES{
  "doxide.yaml",
  "doxide.yml",
  "doxide.json"
};

int main(int argc, char** argv) {
  /* read config file first, so that command-line options can override */
  DriverInitInfo info;
  /* handle command line */
  CliCommand command = CliCommand::NONE;
  CLI::App app{"Modern documentation for modern C++.\n"};
  app.get_formatter()->column_width(30);
  app.add_option("--title",
      info.title,
      "Main page title.");
  app.add_option("--description",
      info.description,
      "Main page description.");
  app.add_option("--output", info.output,
      "Output directory.");
  app.add_option("--coverage", info.coverage,
      "Code coverage file (.gcov or .json).");
  app.add_option("--style", info.style,
      "Specify the style of documentation: plain or mkdocs.");
  app.set_version_flag("--version,-v", PACKAGE_VERSION, "Doxide version.");
  app.fallthrough(true);
  app.add_subcommand("init",
      "Initialize configuration files.")->
      callback([&]() { command = CliCommand::INIT; });
  app.add_subcommand("build",
      "Build documentation in output directory.")->
      callback([&]() { command = CliCommand::BUILD; });
  app.add_subcommand("watch",
      "Watch the documentation's source files and rebuild it on changes.")->
      callback([&]() { command = CliCommand::WATCH; });
  app.add_subcommand("clean",
      "Clean output directory.")->
      callback([&]() { command = CliCommand::CLEAN; });
  app.add_subcommand("cover",
      "Output code coverage data to stdout in JSON format.")->
      callback([&]() { command = CliCommand::COVER; });
  app.require_subcommand(1);
  CLI11_PARSE(app, argc, argv);

  auto iter = std::ranges::find_if(DOXIDE_CONF_FILES, [](const std::string_view& f) {
    return std::filesystem::exists(f);
  });
  std::optional<std::string_view> config_opt = iter != DOXIDE_CONF_FILES.end() ?
    std::optional<std::string_view>{*iter} : std::optional<std::string_view>{};

  std::optional<DriverConfigParser> config_parser;
  if (config_opt) {
    config_parser.emplace(*config_opt);
    config_parser->parse_init_info(info);
  }

  Driver driver(info, config_parser);

  switch (command) {
    case CliCommand::NONE: {
      std::cerr << "No command specified" << std::endl;
      std::exit(1);
      break;
    }
    case CliCommand::INIT: {
      driver.init();
      break;
    }
    case CliCommand::BUILD: {
      driver.build();
      break;
    }
    case CliCommand::WATCH: {
      driver.watch();
      break;
    }
    case CliCommand::CLEAN: {
      driver.clean();
      break;
    }
    case CliCommand::COVER: {
      driver.cover();
      break;
    }
  }
  return 0;
}
