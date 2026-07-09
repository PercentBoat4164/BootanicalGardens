#pragma once

#include <any>
#include <cstdint>
#include <memory>
#include <shared_mutex>

// Every non-virtual child of ComponentColumn has a specific ID
using ComponentId = std::size_t;

/**
 * A component stores data for entities. Each instance of ComponentColumn represents a column within an archetype, where each
 * row contains data for a single entity.
 */
class ComponentColumn {
protected:
    ComponentColumn() = default;
public:
    virtual ~ComponentColumn() = default;
    ComponentColumn(const ComponentColumn&) = delete;
    ComponentColumn(ComponentColumn &&) noexcept = default;
    ComponentColumn & operator=(const ComponentColumn &) = delete;
    ComponentColumn & operator=(ComponentColumn &&) noexcept = default;

    [[nodiscard]] virtual ComponentId getId() const = 0;
    [[nodiscard]] virtual std::size_t getLength() const = 0;

    /**
     * add an element or elements to this component column
     *
     * @param column the items to be appended to this component column
     */
    virtual void add(std::unique_ptr<ComponentColumn> column) = 0;

    /**
     * Get a range of elements from this ComponentColumn. Moves the desired data out of this column into a new
     * ComponentColumn.
     *
     * @param start The index of the first element to get
     * @param size The number of elements to get
     * @return A unique_ptr to a new ComponentColumn holding the gotten data
     */
    virtual std::unique_ptr<ComponentColumn> getRange(size_t start, size_t size) = 0;

    /**
     * Erases an element from the ComponentColumn, moving the last element to take its place. entityRecords should be
     * updated after this is called.
     */
    virtual void remove(size_t index) = 0;
};