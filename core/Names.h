#ifndef SRUBY_NAMES_H
#define SRUBY_NAMES_H

#include "common/common.h"
#include <string>
#include <vector>

#include "absl/strings/string_view.h"

namespace ruby_typer {
namespace core {
class GlobalState;
class Name;
enum NameKind : u1 {
    UTF8 = 1,
    UNIQUE = 2,
    CONSTANT = 3,
};

CheckSize(NameKind, 1, 1);

inline int _NameKind2Id_UTF8(NameKind nm) {
    ENFORCE(nm == UTF8);
    return 1;
}

inline int _NameKind2Id_UNIQUE(NameKind nm) {
    ENFORCE(nm == UNIQUE);
    return 2;
}

inline int _NameKind2Id_CONSTANT(NameKind nm) {
    ENFORCE(nm == CONSTANT);
    return 3;
}

class NameRef final {
public:
    friend GlobalState;
    friend Name;

    NameRef() : _id(-1){};

    // WellKnown is a tag to statically indicate that the caller is deliberately
    // constructing a well-known name, whose ID is stable across all
    // GlobalStates. This should never be used outside of the name constructors
    // generated by tools/generate_names.cc
    struct WellKnown {};

#ifdef DEBUG_MODE
    constexpr NameRef(WellKnown, unsigned int id) : _id(id), globalStateId(-1) {}
#else
    constexpr NameRef(WellKnown, unsigned int id) : _id(id) {}
#endif

    NameRef(const GlobalState &gs, unsigned int id);

    NameRef(const NameRef &nm) = default;

    NameRef(NameRef &&nm) = default;

    NameRef &operator=(const NameRef &rhs) = default;

    bool operator==(const NameRef &rhs) const {
        return _id == rhs._id;
    }

    bool operator!=(const NameRef &rhs) const {
        return !(rhs == *this);
    }

    inline int id() const {
        return _id;
    }

    Name &data(GlobalState &gs) const;
    const Name &data(const GlobalState &gs) const;

    // Returns the `0` NameRef, used to indicate non-existence of a name
    static NameRef noName() {
        return NameRef(WellKnown{}, 0);
    }

    inline bool exists() const {
        return _id != 0;
    }

    NameRef addEq(GlobalState &gs) const;
    NameRef addAt(GlobalState &gs) const;

    std::string toString(const GlobalState &gs) const;
    std::string show(const GlobalState &gs) const;

    bool isWellKnownName() const;

public:
    int _id;

#ifdef DEBUG_MODE
    int globalStateId;
#endif
};

CheckSize(NameRef, 4, 4);

struct RawName final {
    absl::string_view utf8;
};
CheckSize(RawName, 16, 8);

enum UniqueNameKind : u2 { Parser, Desugar, Namer, Singleton, Overload, TypeVarName };

struct UniqueName final {
    NameRef original;
    UniqueNameKind uniqueNameKind;
    u2 num;
};

CheckSize(UniqueName, 8, 4);

struct ConstantName final {
    NameRef original;
};

#include "core/Names_gen.h"

class Name final {
public:
    friend GlobalState;

    NameKind kind;

private:
    unsigned char UNUSED(_fill[3]);

public:
    union { // todo: can discriminate this union through the pointer to Name
        // itself using lower bits
        RawName raw;
        UniqueName unique;
        ConstantName cnst;
    };

    Name() noexcept {};

    Name(Name &&other) noexcept = default;

    Name(const Name &other) = delete;

    ~Name() noexcept;

    bool operator==(const Name &rhs) const;

    bool operator!=(const Name &rhs) const;
    bool isClassName(const GlobalState &gs) const;

    std::string toString(const GlobalState &gs) const;
    std::string show(const GlobalState &gs) const;
    void sanityCheck(const GlobalState &gs) const;
    NameRef ref(const GlobalState &gs) const;

    Name deepCopy(const GlobalState &to) const;

private:
    unsigned int hash(const GlobalState &gs) const;
};

CheckSize(Name, 24, 8);
} // namespace core
} // namespace ruby_typer

template <> struct std::hash<ruby_typer::core::NameRef> {
    size_t operator()(const ruby_typer::core::NameRef &x) const {
        return x._id;
    }
};

template <> struct std::equal_to<ruby_typer::core::NameRef> {
    constexpr bool operator()(const ruby_typer::core::NameRef &lhs, const ruby_typer::core::NameRef &rhs) const {
        return lhs._id == rhs._id;
    }
};

#endif // SRUBY_NAMES_H
