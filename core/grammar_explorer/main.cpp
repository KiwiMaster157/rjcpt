#include "Grammar.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

int main(int aArgc, char** aArgv)
{
#ifdef _WIN32
   _set_error_mode(_OUT_TO_MSGBOX);
#endif

   if (aArgc != 3)
   {
      std::cout << "Expected usage: rjcpt-grammar-test <grammar-file> <output-file>" << std::endl;
      return 1;
   }

   std::cout << "Reading from \"" << aArgv[1] << "\"" << std::endl;
   std::cout << "Writing results to \"" << aArgv[2] << "\"" << std::endl;

   while (true)
   {
      std::ifstream grammarFile(aArgv[1]);
      if (!grammarFile.is_open())
      {
         std::cout << "Unable to open grammar file.";
         return 2;
      }

      std::string grammarText;
      std::getline(grammarFile, grammarText, '\0');
      grammarFile.close();

      rjcpt::lex::Grammar g;
      std::vector<std::string> errors = g.Compile(grammarText);

      std::ofstream fout(aArgv[2]);
      if (!fout.is_open())
      {
         std::cout << "Unable to open output file.";
         return 2;
      }
      if (!errors.empty())
      {
         for (const std::string& error : errors)
         {
            fout << error << std::endl;
         }
      }
      else
      {
         fout << "Grammar compiled without errors." << std::endl;
      }
      fout.close();

      if (grammarText.ends_with("exit()"))
      {
         std::cout << "\nFound \"exit()\" at end of input file. Terminating program." << std::endl;
         return 0;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
   }

   return 0;
}
