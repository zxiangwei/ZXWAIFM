#pragma once

#define SERIALIZE1(cls, f1) \
  inline rpc::Serializer &operator<<(rpc::Serializer &s, const cls &obj) {\
    s << obj.f1;                  \
    return s;                  \
  }                           \
  inline rpc::Serializer &operator>>(rpc::Serializer &s, cls &obj) {\
    s >> obj.f1;                    \
    return s;                    \
  }

#define SERIALIZE2(cls, f1, f2) \
  inline rpc::Serializer &operator<<(rpc::Serializer &s, const cls &obj) {\
    s << obj.f1 << obj.f2;                  \
    return s;                  \
  }                           \
  inline rpc::Serializer &operator>>(rpc::Serializer &s, cls &obj) {\
    s >> obj.f1 >> obj.f2;                    \
    return s;                    \
  }

#define SERIALIZE3(cls, f1, f2, f3) \
  inline rpc::Serializer &operator<<(rpc::Serializer &s, const cls &obj) {\
    s << obj.f1 << obj.f2 << obj.f3;                  \
    return s;                  \
  }                           \
  inline rpc::Serializer &operator>>(rpc::Serializer &s, cls &obj) {\
    s >> obj.f1 >> obj.f2 >> obj.f3;                    \
    return s;                    \
  }

#define SERIALIZE4(cls, f1, f2, f3, f4) \
  inline rpc::Serializer &operator<<(rpc::Serializer &s, const cls &obj) {\
    s << obj.f1 << obj.f2 << obj.f3 << obj.f4;                  \
    return s;                  \
  }                           \
  inline rpc::Serializer &operator>>(rpc::Serializer &s, cls &obj) {\
    s >> obj.f1 >> obj.f2 >> obj.f3 >> obj.f4;                    \
    return s;                    \
  }

#define SERIALIZE5(cls, f1, f2, f3, f4, f5) \
  inline rpc::Serializer &operator<<(rpc::Serializer &s, const cls &obj) {\
    s << obj.f1 << obj.f2 << obj.f3 << obj.f4 << obj.f5;                  \
    return s;                  \
  }                           \
  inline rpc::Serializer &operator>>(rpc::Serializer &s, cls &obj) {\
    s >> obj.f1 >> obj.f2 >> obj.f3 >> obj.f4 >> obj.f5;                    \
    return s;                    \
  }

#define SERIALIZE6(cls, f1, f2, f3, f4, f5, f6) \
  inline rpc::Serializer &operator<<(rpc::Serializer &s, const cls &obj) {\
    s << obj.f1 << obj.f2 << obj.f3 << obj.f4 << obj.f5 << obj.f6;                  \
    return s;                  \
  }                           \
  inline rpc::Serializer &operator>>(rpc::Serializer &s, cls &obj) {\
    s >> obj.f1 >> obj.f2 >> obj.f3 >> obj.f4 >> obj.f5 >> obj.f6;                    \
    return s;                    \
  }

#define SERIALIZE7(cls, f1, f2, f3, f4, f5, f6, f7) \
  inline rpc::Serializer &operator<<(rpc::Serializer &s, const cls &obj) {\
    s << obj.f1 << obj.f2 << obj.f3 << obj.f4 << obj.f5 << obj.f6 << obj.f7;                  \
    return s;                  \
  }                           \
  inline rpc::Serializer &operator>>(rpc::Serializer &s, cls &obj) {\
    s >> obj.f1 >> obj.f2 >> obj.f3 >> obj.f4 >> obj.f5 >> obj.f6 >> obj.f7;                    \
    return s;                    \
  }

#define SERIALIZE8(cls, f1, f2, f3, f4, f5, f6, f7, f8) \
  inline rpc::Serializer &operator<<(rpc::Serializer &s, const cls &obj) {\
    s << obj.f1 << obj.f2 << obj.f3 << obj.f4 << obj.f5 << obj.f6 << obj.f7 << obj.f8;                  \
    return s;                  \
  }                           \
  inline rpc::Serializer &operator>>(rpc::Serializer &s, cls &obj) {\
    s >> obj.f1 >> obj.f2 >> obj.f3 >> obj.f4 >> obj.f5 >> obj.f6 >> obj.f7 >> obj.f8;                    \
    return s;                    \
  }

#define SERIALIZE9(cls, f1, f2, f3, f4, f5, f6, f7, f8, f9) \
  inline rpc::Serializer &operator<<(rpc::Serializer &s, const cls &obj) {\
    s << obj.f1 << obj.f2 << obj.f3 << obj.f4 << obj.f5 << obj.f6 << obj.f7 << obj.f8 << obj.f9;                  \
    return s;                  \
  }                           \
  inline rpc::Serializer &operator>>(rpc::Serializer &s, cls &obj) {\
    s >> obj.f1 >> obj.f2 >> obj.f3 >> obj.f4 >> obj.f5 >> obj.f6 >> obj.f7 >> obj.f8 >> obj.f9;                    \
    return s;                    \
  }

#define SERIALIZE10(cls, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10) \
  inline rpc::Serializer &operator<<(rpc::Serializer &s, const cls &obj) {\
    s << obj.f1 << obj.f2 << obj.f3 << obj.f4 << obj.f5 << obj.f6 << obj.f7 << obj.f8 << obj.f9 << obj.f10;                  \
    return s;                  \
  }                           \
  inline rpc::Serializer &operator>>(rpc::Serializer &s, cls &obj) {\
    s >> obj.f1 >> obj.f2 >> obj.f3 >> obj.f4 >> obj.f5 >> obj.f6 >> obj.f7 >> obj.f8 >> obj.f9 >> obj.f10;                    \
    return s;                    \
  }

