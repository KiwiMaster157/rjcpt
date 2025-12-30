#pragma once

#include <cstdlib>
#include <stdexcept>

namespace rjcpt
{
template<typename T, std::size_t N>
class Stack
{
public:
   std::size_t Size() const { return mCount; }
   constexpr static std::size_t Capacity() { return N; }

   template<typename... Args>
   T& Emplace(Args&&... aArgs)
   {
      Push(T(static_cast<Args&&>(aArgs)...));
      return Top();
   }
   void Push(const T& aValue)
   {
      if (mCount < N)
      {
         mData[mCount] = aValue;
         ++mCount;
      }
      else
      {
         throw std::runtime_error("Stack is full.");
      }
   }
   const T& Top() const
   {
      if (mCount > 0)
      {
         return mData[mCount - 1];
      }
      throw std::logic_error("Empty stack.");
   }
   T& Top()
   {
      if (mCount > 0)
      {
         return mData[mCount - 1];
      }
      throw std::logic_error("Empty stack.");
   }
   const T& Pop()
   {
      if (mCount > 0)
      {
         --mCount;
         return mData[mCount];
      }
      else
      {
         throw std::logic_error("Empty stack.");
      }
   }

private:
   T mData[N];
   std::size_t mCount = 0;
};
}
