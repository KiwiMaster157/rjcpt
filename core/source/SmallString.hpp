#pragma once

#include <array>
#include <compare>
#include <stdexcept>
#include <string>
#include <string_view>

namespace rjcpt
{
   template<std::size_t N>
   class SmallString
   {
   public:
      SmallString() = default;
      template<std::size_t K>
      SmallString(const char(&aValue)[K])
      {
         static_assert(K < N, "String is too long.");
         std::ranges::copy(aValue, mData.begin());
      }
      SmallString(std::string_view aView)
      {
         if (CanConvert(aView))
         {
            std::ranges::copy(aView, mData.begin());
         }
         else
         {
            throw std::runtime_error("Input too long for SmallString.");
         }
      }
      template<std::size_t K>
      explicit(K > N) SmallString(const SmallString<K>& aOther)
      {
         auto sv = aOther.view();
         if (CanConvert(sv))
         {
            std::ranges::copy(sv, mData.begin());
         }
         else
         {
            throw std::runtime_error("Input too long for SmallString.");
         }
      }

      static bool CanConvert(std::string_view aValue)
      {
         return aValue.size() < N;
      }

      std::string_view view() const
      {
         return std::string_view(mData.data());
      }
      std::string str() const
      {
         return std::string(mData.data());
      }

      template<std::size_t R>
      friend bool operator==(const SmallString<N>& aLeft, const SmallString<R>& aRight)
      {
         return aLeft.view() == aRight.view();
      }
      template<std::size_t R>
      friend auto operator<=>(const SmallString<N>& aLeft, const SmallString<R>& aRight)
      {
         return aLeft.view() <=> aRight.view();
      }
   private:
      std::array<char, N> mData = {};
   };
}

template<std::size_t L, std::size_t R>
struct std::common_type<rjcpt::SmallString<L>, rjcpt::SmallString<R>>
{
   using type = rjcpt::SmallString<std::max(L, R)>;
};
