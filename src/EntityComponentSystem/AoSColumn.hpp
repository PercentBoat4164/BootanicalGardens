#pragma once
#include <vector>

#include "AoSColumn.hpp"
#include "ComponentColumn.hpp"

template<ComponentId id, typename T>
class AoSColumn : public ComponentColumn, public std::vector<T> {
public:
    static constexpr ComponentId ID = id;

    explicit AoSColumn(std::vector<T>&& vec) : ComponentColumn(), std::vector<T>(std::move(vec)) {
    }

    AoSColumn() = default;

    ComponentId getId() override {
        return ID;
    }

    std::size_t getLength() override {
        return this->size();
    }

    void add(const std::unique_ptr<ComponentColumn> column) override {
        this->reserve(this->size() + column->getLength());
        AoSColumn* downCastedColumn = static_cast<AoSColumn*>(column.get());
        for (T& i : *downCastedColumn) {
            this->push_back(std::move(i));
        }
    }

    std::unique_ptr<ComponentColumn> getRange(const size_t start, const size_t size) override {
        AoSColumn result;
        // go through backwards so that the order of elements not removed is preserved
        for (size_t i = start + size; i-- > start;) {
            result.emplace_back(std::move(this->at(i)));
            this->remove(i);
        }
        std::unique_ptr<ComponentColumn> out = std::make_unique<AoSColumn>(std::move(result));
        return out;
    }

    void remove(size_t index) override {
        this->at(index) = std::move(this->back());
        this->pop_back();
    }
};