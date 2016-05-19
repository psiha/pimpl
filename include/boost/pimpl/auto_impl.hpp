////////////////////////////////////////////////////////////////////////////////
///
/// \file auto_impl.hpp
/// -------------------
///
/// Copyright (c) Domagoj Saric 2016.
///
/// WIP, wannabe Boost.Pimpl library (@ https://github.com/psiha/pimpl)
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
#ifndef impl_hpp__774D7187_6a61_4941_A036_279F0D6D6CD0
#define impl_hpp__774D7187_6a61_4941_A036_279F0D6D6CD0
#pragma once
//------------------------------------------------------------------------------
#include "auto_pimpl.hpp"

#include <cstdint>
#include <type_traits>
#include <utility>
//------------------------------------------------------------------------------
namespace boost
{
//------------------------------------------------------------------------------
namespace pimpl
{
//------------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
///
/// \struct implementation
///
/// \brief Interface -> implementation mapping metafunction.
/// \details By default maps to the Interface::impl member type. Can be
/// specialized to use a different mapping.
///
////////////////////////////////////////////////////////////////////////////////

template <class Interface>
struct implementation { using type = typename Interface::impl; };


namespace detail
{
    template <std::size_t PlaceholderSize     , std::size_t ImplementationSize     > struct assert_storage_size      { static_assert( PlaceholderSize      >= ImplementationSize     , "Insufficient storage size specified on interface side." ); };
    template <std::size_t PlaceholderAlignment, std::size_t ImplementationAlignment> struct assert_storage_alignment { static_assert( PlaceholderAlignment >= ImplementationAlignment, "Insufficient alignment specified on interface side."    ); };

    template <typename Interface>
    struct is_noexcept
    {
        using impl_t = typename implementation<Interface>::type;

        static constexpr bool default_ = noexcept( impl_t() );
        static constexpr bool copy     = noexcept( impl_t( std::declval<impl_t const>() ) );
        static constexpr bool move     = noexcept( impl_t( impl_t()                     ) );
        template <typename ... Args>
        static constexpr bool construction_from( Args && ... args ) noexcept { return noexcept( impl_t( std::forward<Args>( args )... ) ); }
    };

    /// \note MSVC14u2 chokes on 'different exception specifications' between
    /// declaration and definition (even if the specification at the definition
    /// is more relaxed or evaluates to the same value).
    ///                                       (19.05.2016.) (Domagoj Saric)
#ifdef _MSC_VER
    #define BOOST_PIMPL_NOEXCEPT( Interface, operation )
#else
    #define BOOST_PIMPL_NOEXCEPT( Interface, operation ) noexcept( detail::is_noexcept<Interface>::operation )
#endif // _MSC_VER
} // namespace Detail

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
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::auto_object() noexcept( noexcept( Interface() ) )
{
    using impl_t = typename implementation<Interface>::type;
    new ( &storage ) impl_t();
}

// Move ////////////////////////////////////////////////////////////////////////
template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::auto_object( auto_object && other ) BOOST_PIMPL_NOEXCEPT( Interface, move )
{
    using impl_t = typename implementation<Interface>::type;
    new ( &storage ) impl_t( std::move( other.impl() ) );
}

// Copy ////////////////////////////////////////////////////////////////////////
template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation
>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::auto_object( auto_object const & other ) BOOST_PIMPL_NOEXCEPT( Interface, copy )
{
    using impl_t = typename implementation<Interface>::type;
    new ( &storage ) impl_t( other.impl() );
}

// Generic /////////////////////////////////////////////////////////////////////
template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation
>
template <typename ... Args>
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::auto_object( fwd, Args && ... args ) BOOST_PIMPL_NOEXCEPT( Interface, template construction_from<Args...>( args... ) )
{
    using impl_t = typename implementation<Interface>::type;

    detail::assert_storage_size     <sizeof (           storage   ), sizeof ( impl_t )>();
    detail::assert_storage_alignment<alignof( decltype( storage ) ), alignof( impl_t )>();

    new ( &storage ) impl_t( std::forward<Args>( args )... );
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
auto_object<Interface, SizeOfImplementation, AlignOfImplementation>::~auto_object() noexcept
{
    using impl_t = typename implementation<Interface>::type;
    impl().~impl_t();
}


////////////////////////////////////////////////////////////////////////////////
///
/// typename implementation<Interface>::type       & impl()      ;
/// typename implementation<Interface>::type const & impl() const;
///
/// \brief Returns an implementation instance for an interface instance.
///
////////////////////////////////////////////////////////////////////////////////

template <class Interface, std::uint32_t sz, std::uint8_t al> auto       & auto_object<Interface, sz, al>::impl()       noexcept { return reinterpret_cast<typename implementation<Interface>::type &>( storage ); }
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
}; // class instantiate


#undef BOOST_PIMPL_NOEXCEPT

//------------------------------------------------------------------------------
} // namespace pimpl
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // impl_hpp
