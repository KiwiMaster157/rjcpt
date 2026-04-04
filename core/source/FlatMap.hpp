#pragma once

#include <algorithm>
#include <optional>
#include <vector>

namespace rjcpt
{

template<typename Key, typename Value, typename Compare = std::less<Key>>
class FlatMap
{
public:
   using value_type     = std::pair<Key, Value>;
   using container      = std::vector<value_type>;
   using iterator       = container::iterator;
   using const_iterator = container::const_iterator;
   using subrange       = std::ranges::subrange<iterator>;
   using const_subrange = std::ranges::subrange<const_iterator>;

   struct projection
   {
      const Key& operator()(const value_type& aValue) const
      {
         return aValue.first;
      }
   };

   void set_dirty(std::size_t aIndex)
   {
      mSortedUntil = std::min(mSortedUntil, aIndex);
   }

   std::size_t size() const
   {
      return mData.size();
   }
   bool is_sorted() const
   {
      return mSortedUntil == mData.size();
   }
   void assert_sorted(const char* aMessage) const
   {
      if (!is_sorted())
      {
         throw std::logic_error(aMessage);
      }
   }
   bool is_unique() const
   {
      auto it = std::ranges::adjacent_find(mData, {}, projection());
      return it == mData.end();
   }
   void assert_unique(const char* aMessage) const
   {
      if (!is_unique())
      {
         throw std::logic_error(aMessage);
      }
   }

   iterator begin()
   {
      return mData.begin();
   }
   const_iterator begin() const
   {
      return mData.begin();
   }
   iterator end()
   {
      return mData.end();
   }
   const_iterator end() const
   {
      return mData.end();
   }

   bool contains(const Key& aKey) const
   {
      assert_sorted("contains() called on unsorted FlatMap");
      auto it = std::ranges::lower_bound(mData, aKey, Compare(), projection());
      return it != mData.end();
   }
   std::optional<std::size_t> find_index(const Key& aKey) const
   {
      assert_sorted("contains() called on unsorted FlatMap");
      auto it = std::ranges::lower_bound(mData, aKey, Compare(), projection());
      if (it != mData.end())
      {
         return it - mData.begin();
      }
      return std::nullopt;
   }
   iterator find(const Key& aKey)
   {
      assert_sorted("find() called on unsorted FlatMap");
      return std::ranges::lower_bound(mData, aKey, Compare(), projection());
   }
   const_iterator find(const Key& aKey) const
   {
      assert_sorted("find() called on unsorted FlatMap");
      return std::ranges::lower_bound(mData, aKey, Compare(), projection());
   }
   subrange equal_range(const Key& aKey)
   {
      assert_sorted("equal_range() called on unsorted FlatMap");
      return std::ranges::equal_range(mData, aKey, Compare(), projection());
   }
   const_subrange equal_range(const Key& aKey) const
   {
      assert_sorted("equal_range() called on unsorted FlatMap");
      return std::ranges::equal_range(mData, aKey, Compare(), projection());
   }
   value_type& at(std::size_t aIndex)
   {
      if (aIndex < size())
      {
         return mData[aIndex];
      }
      throw std::logic_error("Index out of bounds.");
   }
   const value_type& at(std::size_t aIndex) const
   {
      if (aIndex < size())
      {
         return mData[aIndex];
      }
      throw std::logic_error("Index out of bounds.");
   }

   void sort()
   {
      std::ranges::stable_sort(mData, Compare());
      mSortedUntil = mData.size();
   }

   value_type& emplace(const Key& aKey, const Value& aValue)
   {
      return mData.emplace_back(aKey, aValue);
   }
   value_type& emplace(const Key& aKey, Value&& aValue)
   {
      return mData.emplace_back(aKey, std::move(aValue));
   }

   void clear()
   {
      mData.clear();
      mSortedUntil = 0;
   }

private:
   std::size_t             mSortedUntil = 0;
   std::vector<value_type> mData;
};

}
