#pragma once
#include <vector>

#include "AoSColumn.hpp"
#include "ComponentColumn.hpp"

template<ComponentId id, typename T>
class AoSColumn : public ComponentColumn, public std::vector<T> {
public:
    static constexpr ComponentId ID = id;

    // template<typename... Args>
    // explicit AoSColumn(Args&&... args) : std::vector<T>(std::forward<Args>(args)...) {}
    explicit AoSColumn(std::vector<T>&& vec) : std::vector<T>(std::move(vec)) {}
    AoSColumn() = default;

    ComponentId getId() override {
        return ID;
    }

    std::size_t getLength() override {
        return this->size();
    }

    /**
     * @todo maybe add emplace(T...) to avoid creating and copying new column
     * @param column
     */
    void add(std::unique_ptr<ComponentColumn> column) override {
        this->reserve(this->size() + column->getLength());
        AoSColumn downCastedColumn = std::move(*static_cast<AoSColumn*>(column.release()));
        for (auto& i : downCastedColumn) {
            this->emplace_back(std::move(i));
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