// Copyright 2013 Sandia Corporation, Albuquerque, NM.

namespace stk {
namespace simd {

struct Double
{

  inline Double() 
    : _data(_mm_setzero_pd()) {
  }

  inline Double(const double* x) 
    : _data(_mm_loadu_pd(x)) {
  }

  inline Double(const double* x, const int offset) 
    :_data(_mm_setr_pd(x[0],x[offset])) {
  }

  template <typename T>
  inline Double(const T x)
    : _data(_mm_set1_pd(double(x))) {
    }

  inline Double(const __m128d& x)
    : _data(x) {
  }

  inline Double(const Double& x)
    : _data(x._data) {
  }

  inline Double(const volatile Double& x)
    : _data(x._data) {
  }

  inline Double& operator= (const Double& x) {
    _data = x._data;
    return *this;
  }

  template <typename T>
  inline Double& operator= (const T x) {
    _data = _mm_set1_pd(double(x));
    return *this;
  }

  inline Double& operator+= (const Double& a) {
    _data = _mm_add_pd(_data,a._data);
    return *this;
  }

  inline void operator+= (const volatile Double& a) volatile {
    _data = _mm_add_pd(_data,a._data);
  }

  inline Double& operator-= (const Double& a) {
    _data = _mm_sub_pd(_data,a._data);
    return *this;
  }

  inline Double& operator*= (const Double& a) {
    _data = _mm_mul_pd(_data,a._data);
    return *this;
  }

  inline Double& operator/= (const Double& a) {
    _data = _mm_div_pd(_data,a._data);
    return *this;
  }

  inline Double& operator+= (const double a) {
    _data = _mm_add_pd(_data,_mm_set1_pd(a));
    return *this;
  }

  inline Double& operator-= (const double a) {
    _data = _mm_sub_pd(_data,_mm_set1_pd(a));
    return *this;
  }

  inline Double& operator*= (const double a) {
    _data = _mm_mul_pd(_data,_mm_set1_pd(a));
    return *this;
  }

  inline Double& operator/= (const double a) {
    _data = _mm_div_pd(_data,_mm_set1_pd(a));
    return *this;
  }

  inline Double operator-() const {
    return Double( _mm_sub_pd(Double(0.0)._data,_data) );
  }

  inline double& operator[](int i) {return (reinterpret_cast<double*>(&_data))[i];}
  inline const double& operator[](int i) const {return (reinterpret_cast<const double*>(&_data))[i];}
    
  inline int64_t& Int(int i) {return (reinterpret_cast<int64_t*>(&_data))[i];}
  inline const int64_t& Int(int i) const {return (reinterpret_cast<const int64_t*>(&_data))[i];}

  inline uint64_t& UInt(int i) {return (reinterpret_cast<uint64_t*>(&_data))[i];}
  inline const uint64_t& UInt(int i) const {return (reinterpret_cast<const uint64_t*>(&_data))[i];}

  __m128d _data; // the "_" means you should try not to use this directly
  // it is made public to avoid function call overhead 
  // and/or so the compiler doesn't have to use up one of
  // inlining depths (usually max inlining depth ~5)
};

} // namespace simd
} // namespace stk
