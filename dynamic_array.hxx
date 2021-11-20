#pragma once
#include <memory>

#include "array_view.hxx"

//
#include "__namespace__.h"

namespace CPPHEADERS_NS_ {

template <typename Ty_>
class dynamic_array : public array_view<Ty_> {
 public:
  dynamic_array(size_t num_elems)  //
          noexcept(std::is_nothrow_default_constructible_v<Ty_>)
          : array_view<Ty_>(new Ty_[num_elems], num_elems),
            _buf(this->data()) {}

  dynamic_array() noexcept = default;
  dynamic_array(dynamic_array&& other) noexcept { *this = std::move(other); }
  dynamic_array& operator=(dynamic_array&& other) noexcept {
    this->array_view<Ty_>::operator=(other);
    _buf                           = std::move(other._buf);
    
    other.array_view<Ty_>::operator=({});
    return *this;
  }

 private:
  std::unique_ptr<Ty_[]> _buf = nullptr;
};

}  // namespace CPPHEADERS_NS_