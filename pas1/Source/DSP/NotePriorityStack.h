#pragma once

#include <array>
#include <cstddef>

namespace pas1::dsp {

struct HeldMidiNote {
    int channel = 1;
    int note = 60;
    float velocity = 0.0f;
};

class NotePriorityStack {
public:
    static constexpr std::size_t capacity = 128;

    bool noteOn(HeldMidiNote note) noexcept {
        const auto alreadyHoldingNotes = size_ > 0;
        for (std::size_t index = 0; index < size_; ++index) {
            if (notes_[index].channel != note.channel || notes_[index].note != note.note)
                continue;
            removeAt(index);
            break;
        }
        if (size_ == notes_.size())
            removeAt(0);
        notes_[size_++] = note;
        return alreadyHoldingNotes;
    }

    bool noteOff(int channel, int note) noexcept {
        for (std::size_t index = size_; index > 0; --index) {
            const auto candidate = index - 1;
            if (notes_[candidate].channel != channel || notes_[candidate].note != note)
                continue;
            removeAt(candidate);
            return true;
        }
        return false;
    }

    const HeldMidiNote* mostRecent() const noexcept {
        return size_ > 0 ? &notes_[size_ - 1] : nullptr;
    }

    std::size_t size() const noexcept { return size_; }
    void clear() noexcept { size_ = 0; }

private:
    void removeAt(std::size_t index) noexcept {
        for (auto move = index + 1; move < size_; ++move)
            notes_[move - 1] = notes_[move];
        --size_;
    }

    std::array<HeldMidiNote, capacity> notes_{};
    std::size_t size_ = 0;
};

} // namespace pas1::dsp
