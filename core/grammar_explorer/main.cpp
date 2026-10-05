#include "Grammar.hpp"
#include "Parser.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

struct TestToken : rjcpt::lex::Token
{
   int mType = -1;
};

class TestCharacterParser : public rjcpt::lex::Parser
{
public:
   explicit TestCharacterParser(const rjcpt::lex::Grammar* aGrammar, std::ostream& aOutStream)
      : rjcpt::lex::Parser(aGrammar)
      , mOutStream(aOutStream)
   {}

   std::vector<rjcpt::lex::Token> ParseString(std::string_view aString)
   {
      mText = aString;
      mTokens.clear();
      mCurrentToken = TestToken();

      auto r = GetGrammar()->FindRule("ENTRY");
      if (!r)
      {
         mOutStream << "ERROR: No grammar rule named ENTRY." << std::endl;
         return {};
      }
      Init(*r);
      for (std::uint32_t i = 0; i < mText.size(); i++)
      {
         PushInput(i, rjcpt::lex::InType(mText[i]));
      }
      EndOfInput(mText.size());

      mText = std::string_view();

      return std::move(mTokens);
   }

protected:
   void Output(std::uint32_t aIndex, rjcpt::lex::InType aValue, rjcpt::lex::OutType aOut) override
   {
      const auto cb = std::to_underlying(aOut);
      if (mCurrentToken.mType >= 0)
      {
         if (cb >= 1000)
         {
            mOutStream << "ERROR: " << aIndex << " " << std::to_underlying(aValue) << ": Unexpected start-of-token." << std::endl;
         }
         else
         {
            if (mCurrentToken.mType != 0 && mCurrentToken.mType != cb)
            {
               mOutStream << "ERROR: " << aIndex << " " << std::to_underlying(aValue)
                  << ": Token type (" << cb << ") does not match expected (" << mCurrentToken.mType << ")." << std::endl;
            }

            mCurrentToken.mSize = aIndex - mCurrentToken.mPosition;
            mOutStream << "TOKEN: " << cb << " " << mCurrentToken.Get(mText) << std::endl;
            mCurrentToken.mType = -1;
            mTokens.push_back(mCurrentToken);
         }
      }
      else
      {
         if (cb >= 1000)
         {
            mCurrentToken.mPosition = aIndex;
            mCurrentToken.mType     = cb - 1000;
         }
         else
         {
            mOutStream << "ERROR: " << aIndex << " " << std::to_underlying(aValue) << ": Unexpected end-of-token." << std::endl;
         }
      }
   }
   void Error(std::uint32_t aIndex, rjcpt::lex::InType aValue, const std::string& aError) override
   {
      mOutStream << "ERROR: " << aIndex << " " << std::to_underlying(aValue) << ": " << aError << std::endl;
   }

private:
   std::ostream& mOutStream;
   std::string_view mText;

   std::vector<rjcpt::lex::Token> mTokens;
   TestToken mCurrentToken;
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
         tcp.ParseString(inputText);
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
