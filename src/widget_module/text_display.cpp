#include "widget_module/widgets/text_display.hpp"
#include "painter_module/painter.hpp"
#include "widget_module/coordinate.hpp"
#include "system_module/events/paint_event.hpp"
#include "system_module/events/resize_event.hpp"
#include <algorithm>
#include <cstddef>
#include <utility>

namespace twf {

Text_display::Text_display(Glyph_string content)
    : contents_{std::move(content)} {
    this->update_display();
}

void Text_display::set_text(const Glyph_string& string) {
    contents_ = string;
    this->update_display();
    this->update();
}

void Text_display::insert(const Glyph_string& string, std::size_t index) {
    if (contents_.empty()) {
        contents_.append(string);
    }
    contents_.insert(std::begin(contents_) + index, std::begin(string),
                     std::end(string));
    this->update_display(this->line_at(index));
}

void Text_display::append(const Glyph_string& string) {
    contents_.append(string);
    this->update_display(this->last_line());
}

void Text_display::erase(std::size_t index, std::size_t length) {
    if (contents_.empty() || index >= contents_.size()) {
        return;
    }
    auto end = std::begin(contents_) + length + index;
    if (length == Glyph_string::npos) {
        end = std::end(contents_);
    }
    contents_.erase(std::begin(contents_) + index, end);
    this->update_display(this->line_at(index));
}

void Text_display::pop_back() {
    if (contents_.empty()) {
        return;
    }
    contents_.pop_back();
    this->update_display(last_line());
}

void Text_display::clear() {
    contents_.clear();
    this->update_display();
}

void Text_display::scroll_up(std::size_t n) {
    if (n > this->top_line()) {
        top_line_ = 0;
    } else {
        top_line_ -= n;
    }
}

void Text_display::scroll_down(std::size_t n) {
    if (this->top_line() + n > this->last_line()) {
        top_line_ = this->last_line();
    } else {
        top_line_ += n;
    }
}

std::size_t Text_display::row_length(std::size_t y) const {
    if (display_state_.empty()) {
        return 0;
    }
    auto line = this->top_line() + y;
    if (line >= display_state_.size()) {
        line = display_state_.size() - 1;
    }
    return display_state_.at(line).length;
}

std::size_t Text_display::string_index(Coordinate position) const {
    return this->string_index(position.x, position.y);
}

std::size_t Text_display::string_index(std::size_t x, std::size_t y) const {
    if (display_state_.empty()) {
        return 0;
    }
    auto line = this->top_line() + y;
    if (line >= display_state_.size()) {
        return Glyph_string::npos;
        // line = display_state_.size() - 1;
    }
    auto info = display_state_.at(line);
    if (info.length == 0) {
        return info.start_index;
    }
    if (x >= info.length) {
        if (line == display_state_.size() - 1) {
            return Glyph_string::npos;
        }
        x = info.length - 1;
    }
    return info.start_index + x;
}

Coordinate Text_display::display_position(std::size_t index) const {
    Coordinate position;
    if (display_state_.empty()) {
        return position;
    }
    if (index >= this->contents_size()) {
        // auto last_x = this->last_index() -
        //               display_state_.at(this->last_line()).start_index + 1;
        const auto last_x = this->row_length(this->last_line());
        const auto last_char = this->glyph_at(this->last_index()).str();
        if (last_char == "\n" || last_x == this->width()) {
            auto offset = 0;
            if (this->cursor_y() == 0 && last_x == this->width()) {
                // (last_char == " " || last_char == "\n")) {
                // ++offset;
                position.y = 0;
            }
            // position.y = this->number_of_rows() - offset;
            else if (this->cursor_y() == 0) {
                position.y = this->number_of_rows() - 1;
            } else {
                position.y = this->number_of_rows();
            }
            position.x = 0;
        } else {
            position.y = this->number_of_rows() - 1;
            position.x = last_x;
        }
    } else {
        // if (this->cursor_y() == 0 && this->top_line() != 0 &&
        //     this->cursor_x() == ) {
        //     position
        // }
        auto line = this->line_at(index);
        position.y = line - this->top_line();
        position.x = index - display_state_.at(line).start_index;
    }
    return position;
}

bool Text_display::paint_event(const Paint_event& event) {
    if (display_state_.empty()) {
        return Widget::paint_event(event);
    }
    Painter p{this};
    std::size_t line_n{0};
    Glyph_string contents_wo_nl{this->contents_};
    std::replace_if(std::begin(contents_wo_nl), std::end(contents_wo_nl),
                    [](auto& g) { return g.str() == "\n"; }, Glyph(" "));

    auto paint = [&p, &line_n, &contents_wo_nl, this](const auto& line) {
        auto sub_begin = std::begin(contents_wo_nl) + line.start_index;
        auto sub_end = sub_begin + line.length;
        p.put(Glyph_string(sub_begin, sub_end), 0, line_n++);
    };
    auto begin = std::begin(display_state_) + this->top_line();
    auto end = std::end(display_state_);
    if (display_state_.size() > this->top_line() + this->height()) {
        end = begin + this->height();
    }
    if (this->top_line() < display_state_.size()) {
        std::for_each(begin, end, paint);
    }
    return Widget::paint_event(event);
}

bool Text_display::resize_event(const Resize_event& event) {
    Widget::resize_event(event);
    this->update_display();
    this->update();
    return true;
}

// TODO: Implement tab character and 'from_line' optimization. consolidate dups.
void Text_display::update_display(std::size_t from_line) {
    display_state_.clear();
    if (this->width() == 0) {
        return;
    }
    std::size_t start_index{0};
    std::size_t length{0};
    std::size_t last_space{0};
    for (std::size_t i{0}; i < contents_.size(); ++i) {
        ++length;
        if (word_wrap_ && contents_.at(i).str() == " ") {
            last_space = length;
        }
        if (contents_.at(i).str() == "\n") {
            display_state_.push_back(line_info{start_index, length});
            start_index += length;
            length = 0;
        } else if (length == this->width()) {
            if (contents_.at(i) == " ") {
                display_state_.push_back(line_info{start_index, length});
                start_index += length;
                length = 0;
            } else if (this->word_wrap() && last_space > 0) {
                i -= length - last_space;
                length = last_space;
                last_space = 0;
                display_state_.push_back(line_info{start_index, length});
                start_index += length;
                length = 0;
            } else {
                display_state_.push_back(line_info{start_index, length});
                start_index += length;
                length = 0;
            }
        }
    }
    if (length != 0) {
        display_state_.push_back(line_info{start_index, length});
    }
    if (display_state_.empty()) {
        return;
    }
    // Scroll if no text on screen
    const auto last_x = this->row_length(this->last_line());
    const auto last_char = this->glyph_at(this->last_index()).str();
    if (last_x == this->width() || last_char == "\n") {
        return;
    }
    if (top_line_ >= display_state_.size()) {
        if (display_state_.empty()) {
            top_line_ = 0;
        }
        top_line_ = display_state_.size() - 1;
    }
}

std::size_t Text_display::line_at(std::size_t index) const {
    if (index >= contents_.size()) {
        return display_state_.empty() ? 0 : display_state_.size() - 1;
    }
    auto contains_index = [index](const auto& info) {
        return (info.start_index + info.length) > index;
    };
    auto line_iter = std::find_if(std::begin(display_state_),
                                  std::end(display_state_), contains_index);
    return line_iter - std::begin(display_state_);
}

std::size_t Text_display::number_of_rows() const {
    if (this->last_line() == 0 || this->top_line() > this->last_line()) {
        return 1;
    }
    auto visible_rows = 1 + this->last_line() - this->top_line();
    if (visible_rows >= this->height()) {
        return this->height();
    }
    return visible_rows;
}

std::size_t Text_display::top_line() const {
    // if (top_line_ > this->last_line()) {
    //     top_line_ = this->last_line();
    // }
    return top_line_;
}

std::size_t Text_display::last_line() const {
    return display_state_.empty() ? 0 : display_state_.size() - 1;
}

std::size_t Text_display::index_at(std::size_t line) const {
    if (display_state_.empty()) {
        return 0;
    }
    if (line >= display_state_.size() && line != 0) {
        // line = display_state_.size() - 1;
        return Glyph_string::npos;
    }
    return display_state_.at(line).start_index;
}

std::size_t Text_display::line_length(std::size_t line) const {
    if (display_state_.empty()) {
        return 0;
    }
    if (line >= display_state_.size()) {
        line = display_state_.size() - 1;
    }
    return display_state_.at(line).length;
}

std::size_t Text_display::last_index() const {
    if (this->contents_empty()) {
        return 0;
    }
    return this->contents_size() - 1;
}

}  // namespace twf
