#include "Grammar.hpp"
#include "CharacterParser.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

struct TestToken : rjcpt::lex::Token
{
   int mType = -1;
};

class TestCharacterParser : public rjcpt::lex::CharacterParser
{
public:
   explicit TestCharacterParser(const rjcpt::lex::Grammar* aGrammar, std::ostream& aOutStream)
      : rjcpt::lex::CharacterParser(aGrammar)
      , mOutStream(aOutStream)
   {}

protected:
   void EmitToken(rjcpt::lex::Token aToken, rjcpt::lex::OutType aType) override
   {
      mOutStream << "TOKEN: " << aToken.Get(mText)
         << " (" << (int) aType << ") line " << aToken.mRow
         << " column " << aToken.mColumn << std::endl;
   }

private:
   std::ostream& mOutStream;
};

int main(int aArgc, char** aArgv)
{
#ifdef _WIN32
   //_set_error_mode(_OUT_TO_MSGBOX);
#endif

   if (aArgc != 4)
   {
      std::cout << "Expected usage: rjcpt-grammar-test <grammar-file> <input-file> <output-file>" << std::endl;
      return 1;
   }

   std::cout << "Reading grammar from \"" << aArgv[1] << "\"" << std::endl;
   std::cout << "Reading input from \"" << aArgv[2] << "\"" << std::endl;
   std::cout << "Writing results to \"" << aArgv[3] << "\"" << std::endl;

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

      std::ofstream fout(aArgv[3]);
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

         std::ifstream inputFile(aArgv[2]);
         if (!inputFile.is_open())
         {
            std::cout << "Unable to open input file.";
            return 2;
         }

         std::string inputText;
         std::getline(inputFile, inputText, '\0');
         inputFile.close();

         TestCharacterParser tcp(&g, fout);
         auto error = tcp.ParseText(inputText, g.FindRule("ENTRY").value());
         if (!error.empty())
         {
            fout << "ERROR: " << error << std::endl;
         }
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
