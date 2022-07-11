#pragma once
#include <memory>

namespace cpph::refl {
class object_metadata;

using std::shared_ptr;
using std::weak_ptr;

using object_metadata_t = object_metadata const*;
using optional_property_metadata = struct property_metadata const*;
using unique_object_metadata = std::unique_ptr<object_metadata const>;

template <typename ValTy_>
struct type_tag;

}  // namespace cpph::refl

#include "detail/object_core_macros.hxx"
