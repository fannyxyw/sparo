#ifndef SIMPLE_COMMAND_LINE_H_
#define SIMPLE_COMMAND_LINE_H_

#include <map>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

class CommandLine {
 public:
#ifdef _WIN32
  using StringType = std::wstring;
#else
  using StringType = std::string;
#endif

  using CharType = StringType::value_type;
  using StringPieceType = std::basic_string_view<CharType>;
  using StringVector = std::vector<StringType>;
  using SwitchMap = std::map<std::string, StringType>;

  // Creates an empty instance for a command line without a program name.
  // Mainly used for building new command lines to launch child processes.
  CommandLine();

  // Constructs from argc/argv.
  CommandLine(int argc, const CharType* const* argv);

  // Constructs from a string vector.
  explicit CommandLine(const StringVector& argv);

#ifdef _WIN32
  // Constructs from a single command line string (e.g., GetCommandLineW()).
  static CommandLine FromString(const StringType& command_line);
#endif

  // Checks if the specified switch exists.
  // The switch name should be lowercase and without a prefix.
  bool HasSwitch(const std::string& switch_string) const;

  // Gets the value associated with the specified switch.
  // Returns an empty string if the switch does not exist or has no value.
  StringType GetSwitchValue(const std::string& switch_string) const;

  // Gets the program path (argv[0]).
  StringType GetProgram() const;

  // Gets all non-switch arguments.
  const StringVector& GetArgs() const;

  // Gets the full command line string, suitable for passing to CreateProcess.
  // On Windows, arguments are quoted correctly.
  StringType GetCommandLineString() const;

  // Gets the argument string (excluding the program itself).
  StringType GetArgumentsString() const;

  // Appends a switch to the command line (optionally with a value).
  void AppendSwitch(const std::string& switch_string);
  void AppendSwitch(const std::string& switch_string, const StringType& value);

  // Appends an argument to the command line.
  void AppendArg(const StringType& value);

 private:
  // Internal version of GetArgumentsString to support quoting on Windows.
  StringType GetArgumentsStringInternal() const;

  // argv array: { program, [(--|-|/)switch[=value]]*, [--], [argument]* }
  StringVector argv_;

  // Parsed switch keys and values.
  SwitchMap switches_;
};

#endif  // SIMPLE_COMMAND_LINE_H_
