Template functions and template class member functions may seem implicitly inlined but they are not inlined. Reason is that during implicit instantiation, they are exempted from one definition rule or ODR, though it violates the rule if there is any template specialization as explicit specialization is not a template. - https://en.cppreference.com/w/cpp/language/definition

Nicolai Josuttis's template book has good literature on inlining template functions( Chapter 9). In most cases, it has zero side effects unless you face foot guns 😆 when 'inline' becomes relevant.
