// Standalone correctness check for the psi.pimpl module (module.cmake, PSI_PIMPL_MODULE): builds
// a small executable that only `import`s psi.pimpl and exercises two real auto_object-based
// pimpl interfaces - one using the default (ordinary, non-trivial) Traits, one using the
// `trivial`-tier Traits added alongside support_level - so a broken module boundary, or a broken
// Traits-constrained special member, shows up as a build or runtime failure here rather than
// surfacing only downstream in a consumer project.
import psi.pimpl;

#include <cstdio>
#include <utility>

namespace
{
    // Default Traits: an ordinary (non-trivial, non-deleted) declared special member set, with
    // noexcept computed dynamically from the Interface - today's/pre-Traits auto_object behavior.
    class widget : public psi::pimpl::auto_object<widget, sizeof( int ), alignof( int )>
    {
    private:
        struct impl_type { int value_; };
    public:
        using implementation = impl_type;

        explicit widget( int const value ) noexcept : auto_object( psi::pimpl::fwd{}, value ) {}
        widget( widget && ) noexcept = default;
        widget( widget const & ) = delete;
        ~widget() noexcept = default;

        int value() const noexcept { return impl().value_; }
    };

    // trivial-tier Traits: moveable/destructor are genuinely `= default`, only correct because
    // impl_type below actually is trivial for those operations (the impl() accessor cross-checks
    // this with static_asserts against the concrete implementation type).
    struct trivial_traits
    {
        static constexpr auto default_constructible = psi::pimpl::support_level::na;
        static constexpr auto copyable               = psi::pimpl::support_level::na;
        static constexpr auto moveable                = psi::pimpl::support_level::trivial;
        static constexpr auto destructor              = psi::pimpl::support_level::trivial;
    };

    class trivial_widget : public psi::pimpl::auto_object<trivial_widget, sizeof( int ), alignof( int ), trivial_traits>
    {
    private:
        struct impl_type { int value_; };
    public:
        using implementation = impl_type;

        explicit trivial_widget( int const value ) noexcept : auto_object( psi::pimpl::fwd{}, value ) {}
        trivial_widget( trivial_widget && ) noexcept = default;
        trivial_widget( trivial_widget const & ) = delete;
        ~trivial_widget() = default;

        int value() const noexcept { return impl().value_; }
    };
} // namespace

int main()
{
    widget w{ 42 };
    int const result{ w.value() };

    widget moved{ std::move( w ) };
    int const moved_result{ moved.value() };

    trivial_widget tw{ 42 };
    int const trivial_result{ tw.value() };

    trivial_widget trivial_moved{ std::move( tw ) };
    int const trivial_moved_result{ trivial_moved.value() };

    std::printf( "%d %d %d %d\n", result, moved_result, trivial_result, trivial_moved_result );
    return
        (          result == 42 ) &&
        (    moved_result == 42 ) &&
        (  trivial_result == 42 ) &&
        ( trivial_moved_result == 42 )
        ? 0 : 1;
}
