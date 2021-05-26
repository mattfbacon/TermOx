#ifndef TERMOX_DEMOS_FOCUS_DEMO_HPP
#define TERMOX_DEMOS_FOCUS_DEMO_HPP
#include <memory>

#include <termox/termox.hpp>

namespace demo {

inline auto focus_box(ox::Focus_policy policy) -> std::unique_ptr<ox::Widget>
{
    using namespace ox::pipe;

    /// Focus_policy to string
    auto const to_string = [](ox::Focus_policy p) -> char32_t const* {
        switch (p) {
            using namespace ox;
            case Focus_policy::None: return U"None";
            case Focus_policy::Tab: return U"Tab";
            case Focus_policy::Click: return U"Click";
            case Focus_policy::Strong: return U"Strong";
            case Focus_policy::Direct: return U"Direct";
        }
        return U"";
    };

    /// Remove tab focus from \p p.
    auto const narrow = [](ox::Focus_policy p) {
        switch (p) {
            using namespace ox;
            case Focus_policy::None:
            case Focus_policy::Tab: return Focus_policy::None;
            case Focus_policy::Click:
            case Focus_policy::Strong: return Focus_policy::Click;
            case Focus_policy::Direct: return Focus_policy::Direct;
        }
        return ox::Focus_policy::None;
    };

    // clang-format off
    auto box_ptr =
        ox::vpair(
            ox::hlabel(to_string(policy))
                | align_center() | ox::pipe::focus(narrow(policy)),
            ox::widget() | ox::pipe::focus(policy)
        ) | bordered();

    box_ptr | direct_focus() | forward_focus(box_ptr->get_children()[1]);

    auto& label = box_ptr->first;
    auto& widg  = box_ptr->second;

    label | forward_focus(widg);

    widg | on_focus_in( [&w = *box_ptr]{ w | walls(fg(ox::Color::Red)); })
         | on_focus_out([&w = *box_ptr]{ w | walls(fg(ox::Color::White)); });
    // clang-format on

    return box_ptr;
}

/// Build a focus app demo and return the owning pointer to it.
inline auto make_focus_demo() -> std::unique_ptr<ox::Widget>
{
    using namespace ox;
    using namespace ox::pipe;

    // clang-format off
    return
    htuple(
        vpair(
            focus_box(Focus_policy::Tab)
                | height_stretch(3),
            hpair(
                focus_box(Focus_policy::Strong),
                focus_box(Focus_policy::Direct)
            )
        ),
        vpair(
            focus_box(Focus_policy::Strong),
            focus_box(Focus_policy::None)
        ),
        vtuple(
            focus_box(Focus_policy::Click),
            htuple(
                focus_box(Focus_policy::Strong),
                vpair(
                    focus_box(Focus_policy::None),
                    focus_box(Focus_policy::Tab)
                ),
                focus_box(Focus_policy::Tab)
            ) | height_stretch(2),
            focus_box(Focus_policy::Strong)
        )
    ) | direct_focus() | on_focus_in([]{
            Terminal::set_palette(ox::amstrad_cpc::palette);
        });
    // clang-format on
}

}  // namespace demo
#endif  // TERMOX_DEMOS_FOCUS_DEMO_HPP
