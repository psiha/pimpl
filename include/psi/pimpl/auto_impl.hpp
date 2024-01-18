////////////////////////////////////////////////////////////////////////////////
///
/// \file auto_impl.hpp
/// -------------------
///
/// Copyright (c) Domagoj Saric 2016 - 2024.
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
} // namespace Detail


template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation
>
template <typename ... Args>
void auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::construct( Args &&... args )
{
    std::construct_at( &impl(), std::forward<Args>( args )... );
}

////////////////////////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////////////////////////

// Default /////////////////////////////////////////////////////////////////////
template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::auto_object()
noexcept( std::is_nothrow_default_constructible_v<Interface> )
{
    static_assert
    (
        std::is_nothrow_default_constructible_v<typename implementation<Interface>::type>
            ==
        std::is_nothrow_default_constructible_v<                        Interface       >,
        "Interface<->impl exception specification mismatch" // TODO add for other functions
    );
    construct();
}

// Move ////////////////////////////////////////////////////////////////////////
template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::auto_object( auto_object && other )
noexcept( std::is_nothrow_move_constructible_v<Interface> )
{
    construct( std::move( other.impl() ) );
}

// Copy ////////////////////////////////////////////////////////////////////////
template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::auto_object( auto_object const & other )
noexcept( std::is_nothrow_copy_constructible_v<Interface> )
{
    construct( other.impl() );
}

// Move assign //////////////////////////////////////////////////////////////////
template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>& auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::operator=( auto_object && other )
noexcept( std::is_nothrow_move_assignable<Interface>::value )
{
    if ( this != &other ) [[ likely ]]
        impl() = std::move( other.impl() );
    return *this;
}

// Copy assign //////////////////////////////////////////////////////////////////
template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>& auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::operator=( auto_object const & other )
noexcept( std::is_nothrow_copy_assignable<Interface>::value )
{
    if ( this != &other ) [[ likely ]]
        impl() = other.impl();

    return *this;
}

// Generic /////////////////////////////////////////////////////////////////////
template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation
>
template <typename ... Args>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::auto_object( fwd, Args && ... args )
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
    std::uint8_t  AlignOfImplementation
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::~auto_object()
PSI_PIMPL_MSVC16_9_WORKAROUND( noexcept( std::is_nothrow_destructible_v<Interface> ) )
{
    using impl_t = typename implementation<Interface>::type;

    detail::assert_storage_size     <sizeof (           storage_   ), sizeof ( impl_t )>();
    detail::assert_storage_alignment<alignof( decltype( storage_ ) ), alignof( impl_t )>();

    std::destroy_at( &impl() );
}


////////////////////////////////////////////////////////////////////////////////
///
/// typename implementation<Interface>::type       & impl()      ;
/// typename implementation<Interface>::type const & impl() const;
///
/// \brief Returns an implementation instance for an interface instance.
///
////////////////////////////////////////////////////////////////////////////////

template <class Interface, std::uint32_t sz, std::uint8_t al> auto       & auto_object<Interface, sz, al>::impl()       noexcept { return *std::launder( reinterpret_cast<typename implementation<Interface>::type *>( &storage_ ) ); }
template <class Interface, std::uint32_t sz, std::uint8_t al> auto const & auto_object<Interface, sz, al>::impl() const noexcept { return const_cast<auto_object &>( *this ).impl(); } ///< \overload


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
