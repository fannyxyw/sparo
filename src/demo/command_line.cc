#include "command_line.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

#ifdef _WIN32
#include <shellapi.h>
#endif

namespace {

// Defines which strings are switch prefixes
#ifdef _WIN32
const CommandLine::CharType* const kSwitchPrefixes[] = {L"--", L"-", L"/"};
const CommandLine::CharType kSwitchTerminator[] = L"--";
const CommandLine::CharType kSwitchValueSeparator = L'=';
#else
const CommandLine::CharType* const kSwitchPrefixes[] = {"--", "-"};
const CommandLine::CharType kSwitchTerminator[] = "--";
const CommandLine::CharType kSwitchValueSeparator = '=';
#endif

// Checks if a string is a switch and extracts its name and value.
bool IsSwitch(const CommandLine::StringType& string, std::string* switch_name,
              CommandLine::StringType* switch_value) {
  switch_name->clear();
  switch_value->clear();

  for (const auto* prefix : kSwitchPrefixes) {
    CommandLine::StringPieceType prefix_view(prefix);
    if (string.rfind(prefix_view, 0) != 0 || string == prefix_view) {
      continue;
    }

    const size_t prefix_length = prefix_view.length();
    if (prefix_length == string.length()) {
      return false;  // Just a prefix, not a switch
    }

    size_t equals_pos = string.find(kSwitchValueSeparator, prefix_length);
    size_t switch_end_pos = string.length();

    if (equals_pos != CommandLine::StringType::npos) {
      switch_end_pos = equals_pos;
    } else {
      // If there is no '=', look for the first non-alphanumeric character as the start of the value
      for (size_t j = prefix_length; j < string.length(); ++j) {
        if (!isalnum(string[j])) {
          switch_end_pos = j;
          equals_pos = j -  1; // Simulate the position of '='
          break;
        }
      }
    }

    CommandLine::StringType switch_key_native =
        string.substr(prefix_length, switch_end_pos - prefix_length);

#ifdef _WIN32
    // On Windows, convert the switch to lowercase UTF-8
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    *switch_name = converter.to_bytes(switch_key_native);
#else
    *switch_name = switch_key_native;
#endif
    std::transform(switch_name->begin(), switch_name->end(),
                   switch_name->begin(), ::tolower);

    if (equals_pos != CommandLine::StringType::npos) {
      *switch_value = string.substr(equals_pos + 1);
    }

    return true;
  }
  return false;
}

#ifdef _WIN32
// Correctly quotes an argument for the Windows command line.
std::wstring QuoteForWin(const std::wstring& arg) {
  // No need to quote if the argument does not contain spaces or tabs and is not empty.
  if (!arg.empty() && arg.find_first_of(L" \t") == std::wstring::npos) {
    return arg;
  }
  // An empty argument needs to be quoted.
  if (arg.empty()) {
    return L"\"\"";
  }

  std::wstring quoted_arg;
  quoted_arg.push_back(L'"');

  for (size_t i = 0; i < arg.size(); ++i) {
    int backslash_count = 0;
    while (i < arg.size() && arg[i] == L'\\') {
      i++;
      backslash_count++;
    }

    if (i == arg.size()) {
      // Backslashes at the end need to be escaped.
      quoted_arg.append(backslash_count * 2, L'\\');
      break;
    }

    if (arg[i] == L'"') {
      // Backslashes before a quote need to be escaped.
      quoted_arg.append(backslash_count * 2 + 1, L'\\');
      quoted_arg.push_back(arg[i]);
    } else {
      // Backslashes in other cases do not need to be escaped.
      quoted_arg.append(backslash_count, L'\\');
      quoted_arg.push_back(arg[i]);
    }
  }

  quoted_arg.push_back(L'"');
  return quoted_arg;
}
#endif

}  // namespace

CommandLine::CommandLine() { argv_.push_back(StringType()); }

CommandLine::CommandLine(int argc, const CharType* const* argv) {
  if (argc > 0) {
    argv_.assign(argv, argv + argc);
  } else {
    argv_.push_back(StringType());
  }

  bool parsing_switches = true;
  for (size_t i = 1; i < argv_.size(); ++i) {
    const StringType& arg = argv_[i];
    if (!parsing_switches) {
      break;
    }

    if (arg == kSwitchTerminator) {
      parsing_switches = false;
      continue;
    }

    std::string switch_name;
    StringType switch_value;
    if (IsSwitch(arg, &switch_name, &switch_value)) {
      // If the switch itself has no value (i.e., no "="), and there are more arguments,
      // and the next argument is not a switch, then the next argument is the value of this switch.
      if (switch_value.empty() && i + 1 < argv_.size()) {
        std::string next_switch_name;
        StringType next_switch_value;
        if (!IsSwitch(argv_[i + 1], &next_switch_name, &next_switch_value)) {
          switches_[switch_name] = argv_[i + 1];
          i++; // Skip the next argument as it has been used as a value
          continue;
        }
      }
      switches_[switch_name] = switch_value; // Otherwise, use the value parsed by IsSwitch (which may be empty)
    } else {
      break;
    }
  }
}

CommandLine::CommandLine(const StringVector& argv) {
  argv_ = argv;
  if (argv_.empty()) {
    argv_.push_back(StringType());
  }

  // `argv_` is now populated. We can reuse the logic from the other
  // constructor by creating a temporary vector of C-style strings.
  std::vector<const CharType*> c_argv;
  for (const auto& arg : argv_) {
    c_argv.push_back(arg.c_str());
  }
  *this = CommandLine(static_cast<int>(c_argv.size()), c_argv.data());
}

#ifdef _WIN32
// static
CommandLine CommandLine::FromString(const StringType& command_line) {
  int argc = 0;
  wchar_t** argv_win = ::CommandLineToArgvW(command_line.c_str(), &argc);
  if (!argv_win) {
    return CommandLine();
  }
  CommandLine cmd(argc, argv_win);
  ::LocalFree(argv_win);
  return cmd;
}
#endif

bool CommandLine::HasSwitch(const std::string& switch_string) const {
  return switches_.count(switch_string) > 0;
}

CommandLine::StringType CommandLine::GetSwitchValue(
    const std::string& switch_string) const {
  auto it = switches_.find(switch_string);
  if (it == switches_.end()) {
    return StringType();
  }
  return it->second;
}

CommandLine::StringType CommandLine::GetProgram() const {
  if (argv_.empty()) {
    return StringType();
  }
  return argv_[0];
}

const CommandLine::StringVector& CommandLine::GetArgs() const {
  return argv_;
}

CommandLine::StringType CommandLine::GetCommandLineString() const {
  StringType command_line_str;
  if (!argv_.empty()) {
#ifdef _WIN32
    command_line_str += QuoteForWin(argv_[0]);
#else
    command_line_str += argv_[0];
#endif
  }

  StringType arguments_str = GetArgumentsStringInternal();
  if (!arguments_str.empty()) {
    command_line_str += ' ';
    command_line_str += arguments_str;
  }
  return command_line_str;
}

CommandLine::StringType CommandLine::GetArgumentsString() const {
  return GetArgumentsStringInternal();
}

void CommandLine::AppendSwitch(const std::string& switch_string) {
  AppendSwitch(switch_string, StringType());
}

void CommandLine::AppendSwitch(const std::string& switch_string,
                               const StringType& value) {
  std::string lower_switch = switch_string;
  std::transform(lower_switch.begin(), lower_switch.end(), lower_switch.begin(),
                 ::tolower);

  switches_[lower_switch] = value;

  StringType switch_with_prefix;
  switch_with_prefix = "--" + lower_switch;

  if (!value.empty()) {
    switch_with_prefix += kSwitchValueSeparator;
    switch_with_prefix += value;
  }
  argv_.emplace_back(switch_with_prefix);
}

void CommandLine::AppendArg(const StringType& value) {
  argv_.push_back(value);
}

CommandLine::StringType CommandLine::GetArgumentsStringInternal() const {
  StringType params;
  bool first = true;
  for (size_t i = 1; i < argv_.size(); ++i) {
    if (!first) {
      params += ' ';
    }
    first = false;

#ifdef _WIN32
    params += QuoteForWin(argv_[i]);
#else
    params += argv_[i];
#endif
  }
  return params;
}
