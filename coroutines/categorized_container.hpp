// (c) 2013 Maciej Gajewski, <maciej.gajewski0@gmail.com>
#ifndef COROUTINES_CATEGORIZED_CONTAINER_HPP
#define COROUTINES_CATEGORIZED_CONTAINER_HPP

#include <vector>
#include <memory>
#include <utility>
#include <cstdint>
#include <cassert>

namespace coroutines {

// Tailor-made container. Stores (and owns) uniq_ptrs to abjects with associated category (small iteger)
template<typename ObjectType, typename CategoryType = std::uint8_t>
class categorized_container
{
public:

    typedef ObjectType* ptr_type;
    typedef std::unique_ptr<ObjectType> unique_ptr_type;


    // insertion with ownership transfer
    void insert(unique_ptr_type&& item, CategoryType category);

    // sets category, aborts where no such item
    void set_category(ptr_type item, CategoryType category);

    // aborts where no such item
    CategoryType get_category(ptr_type item) const;

    // counts items by category
    std::size_t count_category(CategoryType category) const;

    // gets n-th elementh with specified category
    // return nullptr if not found
    ptr_type get_nth(CategoryType category, std::size_t index) const;

    // removes (and destroys) element
    void remove(ptr_type item);

private:
    typedef std::pair<CategoryType, unique_ptr_type> pair_type;
    typedef std::vector<pair_type> container_type;

    typename container_type::iterator find_ptr(ptr_type item)
    {
        return find_if(
            _container.begin(), _container.end(),
            [item](const pair_type& p)
            {
                return p.second.get() == item;
            });
    }

    typename container_type::const_iterator find_ptr(ptr_type item) const
    {
        return find_if(
            _container.begin(), _container.end(),
            [item](const pair_type& p)
            {
                return p.second.get() == item;
            });
    }

    container_type _container;
};

template<typename ObjectType, typename CategoryType>
void categorized_container<ObjectType, CategoryType>::insert(unique_ptr_type&& item, CategoryType category)
{
    _container.push_back(std::make_pair(category, std::move(item)));
}

template<typename ObjectType, typename CategoryType>
void categorized_container<ObjectType, CategoryType>::set_category(ptr_type item, CategoryType category)
{
    auto it = find_ptr(item);
    assert(it != _container.end());

    it->first = category;
}

template<typename ObjectType, typename CategoryType>
CategoryType categorized_container<ObjectType, CategoryType>::get_category(ptr_type item) const
{
    auto it = find_ptr(item);

    assert(it != _container.end());

    return it->first;
}

template<typename ObjectType, typename CategoryType>
std::size_t categorized_container<ObjectType, CategoryType>::count_category(CategoryType category) const
{
    return std::count_if(
        _container.begin(), _container.end(),
        [category](const pair_type& p)
        {
            return category == p.first;
        });
}

template<typename ObjectType, typename CategoryType>
typename categorized_container<ObjectType, CategoryType>::ptr_type
categorized_container<ObjectType, CategoryType>::get_nth(CategoryType category, std::size_t index) const
{
    std::size_t counter = 0;
    auto it = std::find_if(
        _container.begin(), _container.end(),
        [&counter, index, category](const pair_type& p)
        {
            if (p.first == category)
            {
                return counter++ == index;
            }
            else
                return false;
        });

    if (it == _container.end())
    {
        return nullptr;
    }
    else
    {
        return it->second.get();
    }
}

template<typename ObjectType, typename CategoryType>
void categorized_container<ObjectType, CategoryType>::remove(ptr_type item)
{
    auto it = find_ptr(item);
    _container.erase(it);
}


}

#endif
