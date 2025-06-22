#include "DescriptorSetRequirements.hpp"

#include <algorithm>
#include <ranges>

DescriptorSetRequirements DescriptorSetRequirements::operator+(const DescriptorSetRequirements& other) const {
  DescriptorSetRequirements result = *this;
  result += other;
  return result;
}

DescriptorSetRequirements& DescriptorSetRequirements::operator+=(const DescriptorSetRequirements& other) {
  std::map<uint32_t, uint32_t> startIndices;
  for (const auto& [type, descriptorCount]: other.sizes) sizes[type] += descriptorCount;
  for (const auto& [requirer, data] : other.objectDataIndex) {
    if (auto it = objectDataIndex.find(requirer); it != objectDataIndex.end()) {
      // Merge elements found at the location.
      const uint32_t index                                             = it->second.index;
      std::vector<VkDescriptorSetLayoutBinding>& setBinding            = setBindings.at(index);
      const std::vector<VkDescriptorSetLayoutBinding>& otherSetBinding = other.setBindings.at(data.index);
      // Find the total number of bindings
      std::map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
      for (const VkDescriptorSetLayoutBinding& binding : setBinding) bindings.emplace(binding.binding, binding);
      for (const VkDescriptorSetLayoutBinding& binding : otherSetBinding) {
        VkDescriptorSetLayoutBinding& bind = bindings[binding.binding];
        if (bind.stageFlags == 0) bind = binding;
        else {
          bind.stageFlags |= binding.stageFlags;
        }
      }
      setBinding.clear();
      setBinding.reserve(bindings.size());
      for (const VkDescriptorSetLayoutBinding& binding : bindings | std::views::values) setBinding.push_back(binding);
    } else {
      uint32_t& offset = startIndices[data.startIndex];
      if (offset == 0) {
        offset = setBindings.size();
        bindingIDs.resize(offset + data.setCount);
        setBindings.resize(offset + data.setCount);
      }
      const uint32_t index = offset + (data.index - data.startIndex);
      objectDataIndex[requirer] = {offset + (data.index - data.startIndex), offset, data.setCount};
      startIndices.at(data.startIndex) = offset;
      bindingIDs.at(index) = other.bindingIDs.at(data.index);
      setBindings.at(index) = other.setBindings.at(data.index);
    }
  }
  return *this;
}