#pragma once
#include <vector>

#include "AoSColumn.hpp"
#include "AoSColumn.hpp"
#include "AoSColumn.hpp"
#include "ComponentColumn.hpp"

template<ComponentId id, typename T>
class AoSColumn : public ComponentColumn, public std::vector<T> {
public:
    using std::vector<T>::vector;
    using std::vector<T>::operator=;
    static constexpr ComponentId ID = id;

    template<typename... Args>
    AoSColumn(Args&&... args) : std::vector<T>(std::forward<Args>(args)...) {}

    ComponentId getId() override {
        return ID;
    };

    std::size_t getLength() override {
        return this->size();
    }

    void add(ComponentColumn* column) override {
        this->reserve(this->size() + column->getLength());
        for (const auto& i : *static_cast<AoSColumn*>(column)) {
            this->push_back(i);
        }
    }

    std::unique_ptr<ComponentColumn> getRange(const size_t start, const size_t size) override {
        AoSColumn result = std::vector<T>{};
        // go through backwards so that the order of elements not removed is preserved
        for (size_t i = start + size; i-- > start;) {
            result.push_back((*this)[i]);
            this->remove(i);
        }
        std::unique_ptr<ComponentColumn> out = std::make_unique<AoSColumn>(result);
        return out;
    };

    void remove(size_t index) override {
        this->at(index) = (*this).back();
        this->pop_back();
    }
};