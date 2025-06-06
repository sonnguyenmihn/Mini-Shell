#include <iostream>
#include <string>
#include <cstdlib>
#include <unordered_set>
#include <filesystem>
#include <vector>
#include <sstream>

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

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  std::unordered_set<std::string> builtin_commands = {"echo", "type", "exit"};

  while (true)
  {
    std::cout << "$ ";
    std::string input;
    std::getline(std::cin, input);

    // Check if input starts with "exit"
    if (input.substr(0, 4) == "exit")
    {
      // If there's a space after "exit", try to parse the status code
      if (input.length() > 4 && input[4] == ' ')
      {
        try
        {
          int status = std::stoi(input.substr(5));
          if (status >= 0 && status <= 255)
          {
            return status;
          }
        }
        catch (...)
        {
          // If parsing fails, just exit with 0
          return 0;
        }
      }
      return 0;
    }
    else if (input.substr(0, 5) == "echo ")
    {
      std::cout << input.substr(5) << std::endl;
    }
    else if (input.substr(0, 5) == "type ")
    {
      std::string command = input.substr(5);
      if (builtin_commands.find(command) != builtin_commands.end())
      {
        std::cout << command << " is a shell builtin" << std::endl;
      }
      else
      {
        // Get PATH from environment
        const char *path_env = std::getenv("PATH");
        if (path_env)
        {
          std::vector<std::string> path_dirs = split(path_env, ':');
          bool found = false;

          // Search each directory in PATH
          for (const auto &dir : path_dirs)
          {
            std::string full_path = dir + "/" + command;
            if (is_executable(full_path))
            {
              std::cout << command << " is " << full_path << std::endl;
              found = true;
              break;
            }
          }

          if (!found)
          {
            std::cout << command << ": not found" << std::endl;
          }
        }
        else
        {
          std::cout << command << ": not found" << std::endl;
        }
      }
    }
    else
    {
      std::cout << input << ": command not found" << std::endl;
    }
  }
  return 0;
}