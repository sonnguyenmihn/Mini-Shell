#include <iostream>
#include <string>
#include <cstdlib>

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

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
  }
  std::cout << input << ": command not found" << std::endl;
  main();
}