////////////////////////////////////////////////////////////////////////////////
///
/// \file auto_impl.hpp
/// -------------------
///
/// Copyright (c) Domagoj Saric 2016 - 2026.
///
/// Use, modification and distribution is subject to the
/// Boost Software License, Version 1.0.
/// (See accompanying file LICENSE_1_0.txt or copy at
/// http://www.boost.org/LICENSE_1_0.txt)
///
/// For more information, see http://www.boost.org
///
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
#pragma once

#include "auto_pimpl.hpp"

#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
//------------------------------------------------------------------------------
namespace psi::pimpl
{
//------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
///
/// \class implementation
///
/// \brief Interface -> implementation mapping metafunction.
/// \details By default maps to the Interface::implementation member type. Can
/// be specialized to use a different mapping.
///
////////////////////////////////////////////////////////////////////////////////

template <class Interface>
struct implementation { using type = typename Interface::implementation; };


namespace detail
{
    template <std::size_t PlaceholderSize     , std::size_t ImplementationSize     > struct assert_storage_size      { static_assert( PlaceholderSize      >= ImplementationSize     , "Insufficient storage size specified on interface side." ); };
    template <std::size_t PlaceholderAlignment, std::size_t ImplementationAlignment> struct assert_storage_alignment { static_assert( PlaceholderAlignment >= ImplementationAlignment, "Insufficient alignment specified on interface side."    ); };
} // namespace detail


template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation,
    class         Traits
>
template <typename ... Args>
void auto_object<Interface, SizeOfImplementation, AlignOfImplementation, Traits>::construct( Args &&... args )
{
    std::construct_at( &impl(), std::forward<Args>( args )... );
}

////////////////////////////////////////////////////////////////////////////////
// Default constructor
////////////////////////////////////////////////////////////////////////////////

template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation,
    class         Traits
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation, Traits>::auto_object()
noexcept( Traits::default_constructible == support_level::nofail || std::is_nothrow_default_constructible_v<Interface> )
requires ( Traits::default_constructible != support_level::trivial && Traits::default_constructible != support_level::na )
{
    using impl_t = typename implementation<Interface>::type;
    static_assert
    (
        Traits::default_constructible != support_level::nofail || std::is_nothrow_default_constructible_v<impl_t>,
        "Traits::default_constructible == nofail, but the implementation type's default constructor can throw."
    );
    static_assert
    (
        std::is_nothrow_default_constructible_v<impl_t> == std::is_nothrow_default_constructible_v<Interface>,
        "Interface<->impl exception specification mismatch"
    );
    construct();
}

////////////////////////////////////////////////////////////////////////////////
// Move constructor
////////////////////////////////////////////////////////////////////////////////

template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation,
    class         Traits
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation, Traits>::auto_object( auto_object && other )
noexcept( Traits::moveable == support_level::nofail || std::is_nothrow_move_constructible_v<Interface> )
requires ( Traits::moveable != support_level::trivial && Traits::moveable != support_level::na )
{
    static_assert
    (
        Traits::moveable != support_level::nofail
            || std::is_nothrow_move_constructible_v<typename implementation<Interface>::type>,
        "Traits::moveable == nofail, but the implementation type's move constructor can throw."
    );
    construct( std::move( other.impl() ) );
}

////////////////////////////////////////////////////////////////////////////////
// Copy constructor
////////////////////////////////////////////////////////////////////////////////

template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation,
    class         Traits
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation, Traits>::auto_object( auto_object const & other )
noexcept( Traits::copyable == support_level::nofail || std::is_nothrow_copy_constructible_v<Interface> )
requires ( Traits::copyable != support_level::trivial && Traits::copyable != support_level::na )
{
    static_assert
    (
        Traits::copyable != support_level::nofail
            || std::is_nothrow_copy_constructible_v<typename implementation<Interface>::type>,
        "Traits::copyable == nofail, but the implementation type's copy constructor can throw."
    );
    construct( other.impl() );
}

////////////////////////////////////////////////////////////////////////////////
// Move assignment
////////////////////////////////////////////////////////////////////////////////

template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation,
    class         Traits
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation, Traits> &
auto_object<Interface, SizeOfImplementation, AlignOfImplementation, Traits>::operator=( auto_object && other )
noexcept( Traits::moveable == support_level::nofail || std::is_nothrow_move_assignable_v<Interface> )
requires ( !trivially_move_assignable && Traits::moveable != support_level::na )
{
    if ( this != &other ) [[ likely ]]
        impl() = std::move( other.impl() );
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// Copy assignment
////////////////////////////////////////////////////////////////////////////////

template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation,
    class         Traits
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation, Traits> &
auto_object<Interface, SizeOfImplementation, AlignOfImplementation, Traits>::operator=( auto_object const & other )
noexcept( Traits::copyable == support_level::nofail || std::is_nothrow_copy_assignable_v<Interface> )
requires ( !trivially_copy_assignable && Traits::copyable != support_level::na )
{
    if ( this != &other ) [[ likely ]]
        impl() = other.impl();

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// Generic (forwarding) constructor
////////////////////////////////////////////////////////////////////////////////

template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation,
    class         Traits
>
template <typename ... Args>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation, Traits>::auto_object( fwd, Args && ... args )
//noexcept( std::is_nothrow_constructible_v<impl_t, Args...> )
{
    construct( std::forward<Args>( args )... );
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////////////////////////

template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation,
    class         Traits
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation, Traits>::~auto_object()
noexcept( Traits::destructor == support_level::nofail || std::is_nothrow_destructible_v<Interface> )
requires ( Traits::destructor != support_level::trivial )
{
    static_assert
    (
        Traits::destructor != support_level::nofail
            || std::is_nothrow_destructible_v<typename implementation<Interface>::type>,
        "Traits::destructor == nofail, but the implementation type's destructor can throw."
    );

    std::destroy_at( &impl() );
}


////////////////////////////////////////////////////////////////////////////////
///
/// typename implementation<Interface>::type       & impl()      ;
/// typename implementation<Interface>::type const & impl() const;
///
/// \brief Returns an implementation instance for an interface instance.
///
/// \details All Traits-vs-implementation-type consistency checks live here
/// (storage size/alignment, plus every `trivial`-tier cross-check) rather
/// than in the individual special members' bodies: a `Traits::X == trivial`
/// declaration defaults that special member, whose body - where such a check
/// would otherwise live - then never runs, so a false `trivial` claim would
/// silently miscompile (e.g. skip a real destructor call) instead of failing
/// to compile. `impl()` is unconditionally ODR-used by every real accessor
/// regardless of which special-member tier is selected, so it is the one
/// place guaranteed to still catch a mismatched declaration.
///
////////////////////////////////////////////////////////////////////////////////

template <class Interface, std::uint32_t sz, std::uint8_t al, class Traits>
auto & auto_object<Interface, sz, al, Traits>::impl() noexcept
{
    using impl_t = typename implementation<Interface>::type;

    detail::assert_storage_size     <sizeof (           storage_   ), sizeof ( impl_t )>();
    detail::assert_storage_alignment<alignof( decltype( storage_ ) ), alignof( impl_t )>();

    static_assert
    (
        Traits::default_constructible != support_level::trivial
            || std::is_trivially_default_constructible_v<impl_t>,
        "Traits::default_constructible == trivial, but the implementation type's default constructor is not trivial."
    );
    static_assert
    (
        Traits::copyable != support_level::trivial || std::is_trivially_copy_constructible_v<impl_t>,
        "Traits::copyable == trivial, but the implementation type's copy constructor is not trivial."
    );
    static_assert
    (
        Traits::moveable != support_level::trivial || std::is_trivially_move_constructible_v<impl_t>,
        "Traits::moveable == trivial, but the implementation type's move constructor is not trivial."
    );
    static_assert
    (
        Traits::destructor != support_level::trivial || std::is_trivially_destructible_v<impl_t>,
        "Traits::destructor == trivial, but the implementation type's destructor is not trivial."
    );
    static_assert
    (
        !trivially_copy_assignable || std::is_trivially_copy_assignable_v<impl_t>,
        "Traits::copyable/destructor declare trivial copy assignment, but the implementation type's copy assignment is not trivial."
    );
    static_assert
    (
        !trivially_move_assignable || std::is_trivially_move_assignable_v<impl_t>,
        "Traits::moveable/destructor declare trivial move assignment, but the implementation type's move assignment is not trivial."
    );

    return *std::launder( reinterpret_cast<impl_t *>( &storage_ ) );
}
template <class Interface, std::uint32_t sz, std::uint8_t al, class Traits> auto const & auto_object<Interface, sz, al, Traits>::impl() const noexcept { return const_cast<auto_object &>( *this ).impl(); } ///< \overload


////////////////////////////////////////////////////////////////////////////////
///
/// \struct instantiate
///
/// \brief Utility class for explicit instantiations of the compiler-generated
/// Interface/auto_object member functions on the implementation side.
///
/// http://stackoverflow.com/a/3712309/6041906
///
////////////////////////////////////////////////////////////////////////////////

struct instantiate
{
    template <class Interface> struct destructor          : Interface::pimpl_base {         ~destructor(                          ) = default; };
    template <class Interface> struct default_constructor : destructor<Interface> { default_constructor(                          ) = default; };
    template <class Interface> struct copy_constructor    : Interface::pimpl_base {    copy_constructor( copy_constructor const & ) = default; };
    template <class Interface> struct move_constructor    : Interface::pimpl_base {    move_constructor( move_constructor      && ) = default; };
}; // struct instantiate

//------------------------------------------------------------------------------
} // namespace psi::pimpl
//------------------------------------------------------------------------------
