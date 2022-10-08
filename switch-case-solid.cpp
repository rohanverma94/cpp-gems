#include <iostream>
#include <string>
#include <tuple>
#include <functional>

namespace details
{
    /*
        The main problem with the switch (or any) branching is 
        -	it scales bad
        -	it’s against the Open-Closed Principle: the switch algorithm needs to be directly modified
        -	it works only with integral types

        I usually replace it with two lines construct
            
            using operation_t = std::function<void()>;
            template <typename Key>
            using switch_operations_t = std::unordered_map<Key, operation_t>;

        and have decision algorithm that maps the key with operation – remains intact.
        There is only a single place of change – the associative container itself.
        There is no fall through cases, since all keys should be unique.
        
        One can achieve similar with std::tuple and generics

    */
    template <typename Key, typename Tuple, std::size_t...Is>
    constexpr void switch_operationImpl(Key&& key, const Tuple& t, std::index_sequence<Is...>)
    {
        auto check = [key = std::forward<Key>(key)](const auto& t)
        {
            if (key == t.first) {std::invoke(t.second);}
        };
        
        (check(std::get<Is>(t)),...);
    }

    template <typename KeyOperation, typename...KeyOperations>
    constexpr void switch_operation(KeyOperation&& keyOperation, KeyOperations&&...args)
    {
        // Store value/operation pairs by the value
        const auto t = std::make_tuple<std::decay_t<KeyOperations>...>(std::decay_t<decltype(args)>(args)...);
        
        switch_operationImpl(keyOperation, t, std::index_sequence_for<KeyOperations...>{});
    }
}


// Simpliest possible state machine

using states_t = enum class States
    {
        idle = 0,
        starting,
        started,
        failed,
        stopped
    };


void state_machine(states_t state)
{
    #define STRINGIFY(x) #x
    #define print_state(x) std::cout << STRINGIFY(x) << '\n';

    // Map the state with corresponding transition
    // To extend: add only new state/transition pair
    details::switch_operation(state, 
        std::make_pair(states_t::idle, []{print_state(states_t::idle);/*do something useful*/}),
        std::make_pair(states_t::starting, []{print_state(states_t::starting);/*do something useful*/}), 
        std::make_pair(states_t::started, []{print_state(states_t::started);/*do something useful*/}), 
        std::make_pair(states_t::failed, []{print_state(states_t::failed);/*do something useful*/}),
        std::make_pair(states_t::stopped, []{print_state(states_t::stopped);/*do something useful*/})
        );
}


int main()
{
   
    const auto state = states_t::idle;
    state_machine(state);
    state_machine(states_t::starting);
}
