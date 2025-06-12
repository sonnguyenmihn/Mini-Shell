#include <iostream>
#include <string>
#include <cstdlib>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fstream>

// Global variables
const std::unordered_set<std::string> BUILTIN_COMMANDS = {
    "echo", "type", "exit", "cd", "pwd", "export", "unset",
    "alias", "unalias", "source", ".", ":", "true", "false"};

// Cache for command paths
std::unordered_map<std::string, std::string> command_path_cache;

// Function declaration
std::vector<std::string> split(const std::string &str, char delim);
bool is_executable(const std::string &path);
std::string find_command_path(const std::string &command);
void execute_command(const std::string &command, const std::vector<std::string> &args);

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while (true)
  {
    std::cout << "$ ";
    std::string input;
    std::getline(std::cin, input);

    // Split input into command and arguments
    std::vector<std::string> args = split(input, ' ');
    if (args.empty())
      continue;

    std::string command = args[0];

    // Check if it's a builtin command
    if (BUILTIN_COMMANDS.find(command) != BUILTIN_COMMANDS.end())
    {
      if (command == "exit")
      {
        // Handle exit command
        if (args.size() == 2)
        {
          try
          {
            int status = std::stoi(args[1]);
            if (status >= 0 && status <= 255)
            {
              return status;
            }
          }
          catch (...)
          {
            return 0;
          }
        }
        else if (args.size() > 2)
        {
          std::cout << "exit: too many arguments" << std::endl;
        }
        else
        {
          return 0;
        }
      }
      else if (command == "echo")
      {
        if (args.size() > 1)
        {
          std::string message;
          for (size_t i = 1; i < args.size(); ++i)
          {
            if (i > 1)
            {
              // Count spaces between arguments in original input
              size_t space_count = 0;
              size_t pos = input.find(args[i - 1]) + args[i - 1].length();
              while (pos < input.length() && input[pos] == ' ')
              {
                space_count++;
                pos++;
              }
              // Add the same number of spaces
              message.append(space_count, ' ');
            }

            std::string arg = args[i];
            std::string processed;
            bool in_single_quotes = false;
            bool in_double_quotes = false;

            // Process the argument
            for (size_t j = 0; j < arg.length(); ++j)
            {
              // Handle quotes
              if (arg[j] == '\'')
              {
                in_single_quotes = !in_single_quotes;
                continue;
              }
              if (arg[j] == '"')
              {
                in_double_quotes = !in_double_quotes;
                continue;
              }

              // Handle environment variables (only in double quotes or no quotes)
              if (!in_single_quotes && arg[j] == '$' && j + 1 < arg.length())
              {
                size_t var_start = j + 1;
                size_t var_end = var_start;
                while (var_end < arg.length() &&
                       (isalnum(arg[var_end]) || arg[var_end] == '_'))
                {
                  var_end++;
                }
                std::string var_name = arg.substr(var_start, var_end - var_start);
                const char *var_value = std::getenv(var_name.c_str());
                if (var_value)
                {
                  processed += var_value;
                }
                j = var_end - 1;
              }
              else
              {
                processed += arg[j];
              }
            }
            message += processed;
          }
          std::cout << message << std::endl;
        }
      }
      else if (command == "type")
      {
        // Handle type command
        if (args.size() > 1)
        {
          std::string target = args[1];
          if (BUILTIN_COMMANDS.find(target) != BUILTIN_COMMANDS.end())
          {
            std::cout << target << " is a shell builtin" << std::endl;
          }
          else
          {
            std::string path = find_command_path(target);
            if (!path.empty())
            {
              std::cout << target << " is " << path << std::endl;
            }
            else
            {
              std::cout << target << ": not found" << std::endl;
            }
          }
        }
      }
      else if (command == "pwd")
      {
        if (args.size() > 1)
        {
          std::cout << "pwd: too many arguments" << std::endl;
        }
        else
        {
          std::cout << std::filesystem::current_path().string() << std::endl;
        }
      }
      else if (command == "cd")
      {
        if (args.size() > 2)
        {
          std::cout << "cd: too many arguments" << std::endl;
        }
        else if (args.size() == 1)
        {
          std::filesystem::current_path(std::filesystem::path(std::getenv("HOME")));
        }
        else if (args.size() == 2)
        {
          std::string path = args[1];
          // Absolute path
          if (path[0] == '/')
          {
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
            {
              std::filesystem::current_path(path);
            }
            else
            {
              std::cout << "cd: " << path << ": No such file or directory" << std::endl;
            }
          }
          // ~
          else if (path[0] == '~')
          {
            std::filesystem::path home_path = std::filesystem::path(std::getenv("HOME"));
            std::filesystem::path new_path = home_path / path.substr(1);
            if (std::filesystem::exists(new_path) && std::filesystem::is_directory(new_path))
            {
              std::filesystem::current_path(new_path);
            }
            else
            {
              std::cout << "cd: " << path << ": No such file or directory" << std::endl;
            }
          }
          // Relative path
          else
          {
            std::filesystem::path current_path = std::filesystem::current_path();
            std::filesystem::path new_path = current_path / path;
            if (std::filesystem::exists(new_path) && std::filesystem::is_directory(new_path))
            {
              std::filesystem::current_path(new_path);
            }
            else
            {
              std::cout << "cd: " << path << ": No such file or directory" << std::endl;
            }
          }
        }
      }
    }
    else
    {
      // Not a builtin command, check if it's an executable in PATH
      std::string path = find_command_path(command);
      if (command == "cat")
      {
        // Process arguments to handle quotes
        for (size_t i = 1; i < args.size(); ++i)
        {
          std::string arg = args[i];
          std::string processed;
          bool in_single_quotes = false;
          bool in_double_quotes = false;

          // Process the argument to remove quotes
          for (size_t j = 0; j < arg.length(); ++j)
          {
            if (arg[j] == '\'')
            {
              in_single_quotes = !in_single_quotes;
              continue;
            }
            if (arg[j] == '"')
            {
              in_double_quotes = !in_double_quotes;
              continue;
            }
            processed += arg[j];
          }
          args[i] = processed; // Replace the argument with processed version
        }
      }
      if (!path.empty())
      {
        execute_command(command, args);
      }
      else
      {
        std::cout << command << ": command not found" << std::endl;
      }
    }
  }
  return 0;
}

// Function to split string by delimiter
std::vector<std::string> split(const std::string &str, char delim)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(str);
  while (std::getline(tokenStream, token, delim))
  {
    if (!token.empty())
    {
      tokens.push_back(token);
    }
  }
  return tokens;
}

// Function to check if file exists and is executable
bool is_executable(const std::string &path)
{
  std::filesystem::path file_path(path);
  return std::filesystem::exists(file_path) &&
         (std::filesystem::status(file_path).permissions() & std::filesystem::perms::owner_exec) != std::filesystem::perms::none;
}

// Function to find command path, using cache if available
std::string find_command_path(const std::string &command)
{
  // Check cache first
  auto it = command_path_cache.find(command);
  if (it != command_path_cache.end())
  {
    return it->second;
  }

  // If not in cache, search PATH
  const char *path_env = std::getenv("PATH");
  if (path_env)
  {
    std::vector<std::string> path_dirs = split(path_env, ':');

    for (const auto &dir : path_dirs)
    {
      std::string full_path = dir + "/" + command;
      if (is_executable(full_path))
      {
        // Add to cache
        command_path_cache[command] = full_path;
        return full_path;
      }
    }
  }

  return ""; // Command not found
}

void execute_command(const std::string &command, const std::vector<std::string> &args)
{
  pid_t pid = fork();
  if (pid == -1)
  {
    std::cerr << "fork failed" << std::endl;
    return;
  }
  else if (pid == 0)
  {
    // Child process
    std::vector<char *> args_array;
    args_array.reserve(args.size() + 1);

    // Convert strings to char* and store them
    for (const auto &arg : args)
    {
      char *arg_copy = (char *)malloc(arg.length() + 1); // +1 for null terminator
      if (arg_copy == nullptr)
      {
        std::cerr << "Memory allocation failed" << std::endl;
        exit(1);
      }
      strcpy(arg_copy, arg.c_str());
      args_array.push_back(arg_copy);
    }
    args_array.push_back(nullptr); // Null terminate the array

    // Execute the command
    execvp(command.c_str(), args_array.data());

    // If execvp returns, it means it failed
    std::cerr << "execvp failed" << std::endl;

    // Free allocated memory
    for (char *arg : args_array)
    {
      if (arg != nullptr)
      {
        free(arg);
      }
    }
    exit(1);
  }
  else
  {
    // Parent process
    int status;
    waitpid(pid, &status, 0);
  }
}