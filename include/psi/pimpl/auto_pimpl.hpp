////////////////////////////////////////////////////////////////////////////////
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

#include <cstdint>
#include <type_traits>
//------------------------------------------------------------------------------
namespace psi::pimpl
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
/// \enum support_level
///
/// \brief Declared guarantee for a special member operation on the
/// pimpl-erased implementation type, mirroring psi::functionoid's Traits
/// system (see psi/functionoid/policies.hpp):
///
///   na        - the operation is unavailable; the corresponding auto_object
///               special member is deleted, making the fact visible to the
///               type system instead of failing (or silently miscompiling)
///               on use.
///   supported - an ordinary declared special member; noexcept-ness is
///               computed dynamically from the Interface's own
///               std::is_nothrow_*_v (today's behavior, and the default when
///               no Traits are supplied - existing pimpl-based interfaces are
///               unaffected).
///   nofail    - like `supported`, but the noexcept-specifier is asserted
///               true unconditionally; a static_assert cross-checks that the
///               Interface actually is nothrow for that operation (catches a
///               false declaration at compile time rather than silently
///               emitting an incorrect noexcept(true)).
///   trivial   - the corresponding auto_object special member is `= default`
///               on its (only) declaration, making it genuinely trivial and
///               visible to std::is_trivially_*_v (and anything built on it -
///               trivial relocation, memcpy-based optimizations, etc). Only
///               correct when the actual implementation type is itself
///               trivial for that operation; pair with a static_assert at
///               the point where the concrete implementation type is known
///               (see `implementation<Interface>::type` cross-checks in the
///               .cpp defining the Interface's out-of-line special members).
///
////////////////////////////////////////////////////////////////////////////////

enum struct support_level : std::uint8_t
{
    na        = 0,
    supported = 1,
    nofail    = 2,
    trivial   = 3
};

template <support_level Level>
using support_level_t = std::integral_constant<support_level, Level>;


////////////////////////////////////////////////////////////////////////////////
///
/// \struct default_traits
///
/// \brief Traits reproducing auto_object's pre-Traits behavior exactly: every
/// special member is an ordinary (non-trivial, non-deleted) declared
/// function, noexcept computed dynamically from the Interface. Existing
/// pimpl-based interfaces that do not supply a Traits argument get this,
/// unchanged.
///
////////////////////////////////////////////////////////////////////////////////

struct default_traits
{
    static constexpr auto default_constructible = support_level::supported;
    static constexpr auto copyable               = support_level::supported;
    static constexpr auto moveable                = support_level::supported;
    static constexpr auto destructor              = support_level::supported;
};


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
    std::uint8_t  AlignOfImplementation = alignof( void * ),
    class         Traits                = default_traits
>
class [[ clang::trivial_abi ]] auto_object // the derived class can 'cancel' but not enable the trivial_abi attribute
{
public:
    using pimpl_base = auto_object;
    using traits     = Traits;

    template <typename ... Args>
    auto_object( fwd, Args && ... );

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
    /// Each special member below is declared as up to three mutually
    /// exclusive, Traits-constrained overloads (na -> deleted, trivial ->
    /// defaulted, otherwise -> the ordinary declared function) rather than
    /// one function gated by runtime logic - constrained special members are
    /// a C++20 feature; the constraints are spelled mutually exclusive
    /// rather than relying on the more-constrained-wins tie-breaker (GCC and
    /// Clang disagree on it for special members). Mirrors
    /// psi::functionoid::callable (see functionoid.hpp).

    // Default constructor /////////////////////////////////////////////////////
    auto_object() requires ( Traits::default_constructible == support_level::trivial ) = default;
    auto_object() requires ( Traits::default_constructible == support_level::na      ) = delete;
    auto_object()
        noexcept( Traits::default_constructible == support_level::nofail || std::is_nothrow_default_constructible_v<Interface> )
        requires ( Traits::default_constructible != support_level::trivial && Traits::default_constructible != support_level::na );

    // Move constructor /////////////////////////////////////////////////////////
    auto_object( auto_object && ) noexcept requires ( Traits::moveable == support_level::trivial ) = default;
    auto_object( auto_object && ) requires ( Traits::moveable == support_level::na ) = delete;
    auto_object( auto_object && )
        noexcept( Traits::moveable == support_level::nofail || std::is_nothrow_move_constructible_v<Interface> )
        requires ( Traits::moveable != support_level::trivial && Traits::moveable != support_level::na );

    // Copy constructor /////////////////////////////////////////////////////////
    auto_object( auto_object const & ) requires ( Traits::copyable == support_level::trivial ) = default;
    auto_object( auto_object const & ) requires ( Traits::copyable == support_level::na      ) = delete;
    auto_object( auto_object const & )
        noexcept( Traits::copyable == support_level::nofail || std::is_nothrow_copy_constructible_v<Interface> )
        requires ( Traits::copyable != support_level::trivial && Traits::copyable != support_level::na );

    // Destructor ///////////////////////////////////////////////////////////////
    // (no `na` tier - every type must remain destructible.)
   ~auto_object() requires ( Traits::destructor == support_level::trivial ) = default;
   ~auto_object()
        noexcept( Traits::destructor == support_level::nofail || std::is_nothrow_destructible_v<Interface> )
        requires ( Traits::destructor != support_level::trivial );

    // Assignment ///////////////////////////////////////////////////////////////
    // A defaulted (trivial) assignment overwrites the previous target without
    // destroying it, hence the extra `destructor == trivial` requirement -
    // matches psi::functionoid::callable::operator=.
    static bool constexpr trivially_copy_assignable{ Traits::copyable == support_level::trivial && Traits::destructor == support_level::trivial };
    static bool constexpr trivially_move_assignable{ Traits::moveable == support_level::trivial && Traits::destructor == support_level::trivial };

    auto_object & operator=( auto_object const & ) requires ( trivially_copy_assignable ) = default;
    auto_object & operator=( auto_object const & ) requires ( Traits::copyable == support_level::na ) = delete;
    auto_object & operator=( auto_object const & )
        noexcept( Traits::copyable == support_level::nofail || std::is_nothrow_copy_assignable_v<Interface> )
        requires ( !trivially_copy_assignable && Traits::copyable != support_level::na );

    auto_object & operator=( auto_object && ) noexcept requires ( trivially_move_assignable ) = default;
    auto_object & operator=( auto_object && ) requires ( Traits::moveable == support_level::na ) = delete;
    auto_object & operator=( auto_object && )
        noexcept( Traits::moveable == support_level::nofail || std::is_nothrow_move_assignable_v<Interface> )
        requires ( !trivially_move_assignable && Traits::moveable != support_level::na );

    auto       & impl()       noexcept;
    auto const & impl() const noexcept;

private:
    template < typename ... Args > void construct( Args &&... );

    struct alignas( AlignOfImplementation ) storage_t { unsigned char raw_bytes[ SizeOfImplementation ]; };
    storage_t storage_;
}; // class auto_object

#ifdef _MSC_VER
#   pragma warning( pop )
#endif // _MSC_VER

//------------------------------------------------------------------------------
} // namespace psi::pimpl
//------------------------------------------------------------------------------
