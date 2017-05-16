////////////////////////////////////////////////////////////////////////////////
///
/// \file auto_pimpl.hpp
/// --------------------
///
/// Copyright (c) Domagoj Saric 2016 - 2017.
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
#ifndef pimpl_hpp__F7E7B64A_FE5D_44E9_8F3E_5B28E3DDA881
#define pimpl_hpp__F7E7B64A_FE5D_44E9_8F3E_5B28E3DDA881
#pragma once
//------------------------------------------------------------------------------
#include <cstdint>
#include <type_traits>
//------------------------------------------------------------------------------
namespace boost
{
//------------------------------------------------------------------------------
namespace pimpl
{
//------------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
///
/// \class fwd
///
/// \brief Tag struct for the 'perfect forwarding' generic constructor (to
/// prevent it from interfering with overload resolution for the 'standard'
/// constructors).
/// \details Handling these issues with SFINAE and allowing the client interface
/// class to simply import the library provided constructors with a using
/// directive fails due to language defects:
/// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4429.html
/// https://llvm.org/bugs/show_bug.cgi?id=20173
///
/// Thanks to https://probablydance.com/2013/10/05/type-safe-pimpl-implementation-without-overhead
/// for the idea.
///
////////////////////////////////////////////////////////////////////////////////

struct fwd {};


////////////////////////////////////////////////////////////////////////////////
///
/// \class auto_object
///
/// \brief Pimpl template for classes that should be instantiable in automatic
/// storage.
///
////////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#   pragma warning( push )
#   pragma warning( disable : 4324 ) // Structure was padded due to alignment specifier.
#endif // _MSC_VER

template
<
    class Interface,
    std::uint32_t SizeOfImplementation,
    std::uint8_t  AlignOfImplementation = sizeof( void * )
>
class auto_object
{
public:
    using pimpl_base = auto_object;

protected:
    /// <VAR>Interface</VAR> has to explicitly declare its constructors,
    /// destructor and assignment operators (if it uses/provides them) and their
    /// respective exception specification or deleted status (otherwise
    /// compilation fails here due to infinite template instantiation recursion)
    /// - this is by design:
    /// * to force API authors to write self-documenting interfaces
    /// * to prevent the compiler from assuming all of the defaultable functions
    /// (except the destructor) to be possibly throwing
    /// * to enable terse/defaulted noexcept implementations (e.g.:
    /// Impl::Impl( Impl && ) noexcept = default; instead of
    /// Impl::Impl( Impl && other ) noexcept : pimpl_base( std::move( other )
    /// {}).
    ///
    /// \note MSVC's (tested@14.1) is_*_constructible type traits do not work
    /// here (with incomplete types).
    ///                                       (11.05.2017.) (Domagoj Saric)
    auto_object(                      ) noexcept( noexcept( Interface() ) );
    auto_object( auto_object       && ) noexcept( noexcept( Interface( Interface() ) ) );
    auto_object( auto_object const  & ) noexcept( noexcept( Interface( std::declval<Interface const>() ) ) );
    template <typename ... Args>
    auto_object( fwd, Args && ...     );
   ~auto_object(                      ) noexcept( std::is_nothrow_destructible<Interface>::value );

    auto_object& operator=( auto_object       && ) noexcept( std::is_nothrow_move_assignable<Interface>::value );
    auto_object& operator=( auto_object const  & ) noexcept( std::is_nothrow_copy_assignable<Interface>::value );

    auto       & impl()       noexcept;
    auto const & impl() const noexcept;

private:
    struct alignas( AlignOfImplementation ) storage_t { unsigned char raw_bytes[ SizeOfImplementation ]; };
    storage_t storage;
}; // class auto_object

#ifdef _MSC_VER
#   pragma warning( pop )
#endif // _MSC_VER

//------------------------------------------------------------------------------
} // namespace pimpl
//------------------------------------------------------------------------------
} // namespace boost
//------------------------------------------------------------------------------
#endif // pimpl_hpp
