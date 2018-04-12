#include "absl/base/casts.h"
#include "absl/strings/escaping.h"
#include "common/common.h"
#include "core/Context.h"
#include "core/Names.h"
#include "core/TypeConstraint.h"
#include "core/Types.h"

using namespace ruby_typer;
using namespace core;
using namespace std;

string core::ClassType::toString(const GlobalState &gs, int tabs) {
    return this->symbol.data(gs).show(gs);
}

string core::ClassType::show(const GlobalState &gs) {
    return this->symbol.data(gs).show(gs);
}

string core::ClassType::typeName() {
    return "ClassType";
}

string LiteralType::typeName() {
    return "LiteralType";
}

string LiteralType::toString(const GlobalState &gs, int tabs) {
    return this->underlying->toString(gs, tabs) + "(" + showValue(gs) + ")";
}

string LiteralType::show(const GlobalState &gs) {
    return this->underlying->show(gs) + "(" + showValue(gs) + ")";
}

string LiteralType::showValue(const GlobalState &gs) {
    string value;
    SymbolRef undSymbol = cast_type<ClassType>(this->underlying.get())->symbol;
    if (undSymbol == Symbols::String()) {
        value = "\"" + NameRef(gs, this->value).toString(gs) + "\"";
    } else if (undSymbol == Symbols::Symbol()) {
        value = ":\"" + NameRef(gs, this->value).toString(gs) + "\"";
    } else if (undSymbol == Symbols::Integer()) {
        value = to_string(this->value);
    } else if (undSymbol == Symbols::Float()) {
        value = to_string(absl::bit_cast<double>(this->value));
    } else if (undSymbol == Symbols::TrueClass()) {
        value = "true";
    } else if (undSymbol == Symbols::FalseClass()) {
        value = "false";
    } else {
        Error::raise("should not be reachable");
    }
    return value;
}

string TupleType::typeName() {
    return "TupleType";
}

string ShapeType::typeName() {
    return "ShapeType";
}

string MagicType::typeName() {
    return "MagicType";
}

string AliasType::typeName() {
    return "AliasType";
}

string AndType::typeName() {
    return "AndType";
}

string OrType::typeName() {
    return "OrType";
}

void printTabs(stringstream &to, int count) {
    int i = 0;
    while (i < count) {
        to << "  ";
        i++;
    }
}

string TupleType::toString(const GlobalState &gs, int tabs) {
    stringstream buf;
    buf << "TupleType {" << '\n';
    int i = -1;
    for (auto &el : this->elems) {
        i++;
        printTabs(buf, tabs + 1);
        buf << i << " = " << el->toString(gs, tabs + 3) << '\n';
    }
    printTabs(buf, tabs);
    buf << "}";
    return buf.str();
}

string TupleType::show(const GlobalState &gs) {
    stringstream buf;
    buf << "[";
    bool first = true;
    for (auto &el : this->elems) {
        if (first) {
            first = false;
        } else {
            buf << ", ";
        }
        buf << el->show(gs);
    }
    buf << "]";
    return buf.str();
}

string ShapeType::toString(const GlobalState &gs, int tabs) {
    stringstream buf;
    buf << "ShapeType {" << '\n';
    auto valueIterator = this->values.begin();
    for (auto &el : this->keys) {
        printTabs(buf, tabs + 1);
        buf << el->toString(gs, tabs + 2) << " => " << (*valueIterator)->toString(gs, tabs + 2) << '\n';
        ++valueIterator;
    }
    printTabs(buf, tabs);
    buf << "}";
    return buf.str();
}

string ShapeType::show(const GlobalState &gs) {
    stringstream buf;
    buf << "{";
    auto valueIterator = this->values.begin();
    bool first = true;
    for (auto &key : this->keys) {
        if (first) {
            first = false;
        } else {
            buf << ", ";
        }
        SymbolRef undSymbol = cast_type<ClassType>(key->underlying.get())->symbol;
        if (undSymbol == Symbols::Symbol()) {
            buf << NameRef(gs, key->value).toString(gs) << ": ";
        } else {
            buf << key->show(gs) << " => ";
        }
        buf << (*valueIterator)->show(gs);
        ++valueIterator;
    }
    buf << "}";
    return buf.str();
}

string MagicType::toString(const GlobalState &gs, int tabs) {
    return underlying->toString(gs, tabs);
}

string MagicType::show(const GlobalState &gs) {
    return underlying->show(gs);
}

string AliasType::toString(const GlobalState &gs, int tabs) {
    stringstream buf;
    buf << "AliasType { symbol = " << this->symbol.data(gs).fullName(gs) << " }";
    return buf.str();
}

string AliasType::show(const GlobalState &gs) {
    stringstream buf;
    buf << "<Alias:" << this->symbol.data(gs).fullName(gs) << ">";
    return buf.str();
}

string AndType::toString(const GlobalState &gs, int tabs) {
    stringstream buf;
    bool leftBrace = isa_type<OrType>(this->left.get());
    bool rightBrace = isa_type<OrType>(this->right.get());

    if (leftBrace) {
        buf << "(";
    }
    buf << this->left->toString(gs, tabs + 2);
    if (leftBrace) {
        buf << ")";
    }
    buf << " & ";
    if (rightBrace) {
        buf << "(";
    }
    buf << this->right->toString(gs, tabs + 2);
    if (rightBrace) {
        buf << ")";
    }
    return buf.str();
}

string AndType::show(const GlobalState &gs) {
    stringstream buf;

    buf << "T.all(";
    buf << this->left->show(gs);
    buf << ", ";
    buf << this->right->show(gs);
    buf << ")";
    return buf.str();
}

string OrType::toString(const GlobalState &gs, int tabs) {
    stringstream buf;
    bool leftBrace = isa_type<AndType>(this->left.get());
    bool rightBrace = isa_type<AndType>(this->right.get());

    if (leftBrace) {
        buf << "(";
    }
    buf << this->left->toString(gs, tabs + 2);
    if (leftBrace) {
        buf << ")";
    }
    buf << " | ";
    if (rightBrace) {
        buf << "(";
    }
    buf << this->right->toString(gs, tabs + 2);
    if (rightBrace) {
        buf << ")";
    }
    return buf.str();
}

string showOrs(const GlobalState &gs, shared_ptr<Type> left, shared_ptr<Type> right) {
    stringstream buf;
    auto *lt = cast_type<OrType>(left.get());
    if (lt != nullptr) {
        buf << showOrs(gs, lt->left, lt->right);
    } else {
        buf << left->show(gs);
    }
    buf << ", ";

    auto *rt = cast_type<OrType>(right.get());
    if (rt != nullptr) {
        buf << showOrs(gs, rt->left, rt->right);
    } else {
        buf << right->show(gs);
    }
    return buf.str();
}

string showOrSpecialCase(const GlobalState &gs, shared_ptr<Type> type, shared_ptr<Type> rest) {
    auto *ct = cast_type<ClassType>(type.get());
    if (ct != nullptr && ct->symbol == core::Symbols::NilClass()) {
        stringstream buf;
        buf << "T.nilable(";
        buf << rest->show(gs);
        buf << ")";
        return buf.str();
    }
    return "";
}

string OrType::show(const GlobalState &gs) {
    stringstream buf;

    string ret;
    if (!(ret = showOrSpecialCase(gs, this->left, this->right)).empty()) {
        return ret;
    }
    if (!(ret = showOrSpecialCase(gs, this->right, this->left)).empty()) {
        return ret;
    }

    buf << "T.any(";
    buf << showOrs(gs, this->left, this->right);
    buf << ")";
    return buf.str();
}

std::string TypeVar::toString(const GlobalState &gs, int tabs) {
    return "TypeVar(" + sym.data(gs).name.toString(gs) + ")";
}

std::string TypeVar::show(const GlobalState &gs) {
    return sym.data(gs).name.toString(gs);
}

std::string TypeVar::typeName() {
    return "TypeVar";
}

std::string AppliedType::toString(const GlobalState &gs, int tabs) {
    stringstream buf;
    buf << "AppliedType {" << '\n';
    printTabs(buf, tabs + 1);
    buf << "klass = " << this->klass.data(gs).fullName(gs) << '\n';

    printTabs(buf, tabs + 1);
    buf << "targs = [" << '\n';
    int i = -1;
    for (auto &targ : this->targs) {
        ++i;
        printTabs(buf, tabs + 2);
        auto tyMem = this->klass.data(gs).typeMembers()[i];
        buf << tyMem.data(gs).name.toString(gs) << " = " << targ->toString(gs, tabs + 3) << '\n';
    }
    printTabs(buf, tabs + 1);
    buf << "]" << '\n';

    printTabs(buf, tabs);
    buf << "}";
    return buf.str();
}

std::string AppliedType::show(const GlobalState &gs) {
    stringstream buf;
    if (this->klass == core::Symbols::Array()) {
        buf << "T::Array";
    } else if (this->klass == core::Symbols::Hash()) {
        buf << "T::Hash";
    } else {
        buf << this->klass.data(gs).show(gs);
    }
    buf << "[";

    bool first = true;
    for (auto &targ : this->targs) {
        if (this->klass == core::Symbols::Hash() && &targ == &this->targs.back()) {
            break;
        }
        if (first) {
            first = false;
        } else {
            buf << ", ";
        }
        buf << targ->show(gs);
    }

    buf << "]";
    return buf.str();
}

std::string AppliedType::typeName() {
    return "AppliedType";
}

std::string LambdaParam::toString(const GlobalState &gs, int tabs) {
    return "LambdaParam(" + this->definition.data(gs).fullName(gs) + ")";
}

std::string LambdaParam::show(const GlobalState &gs) {
    return this->definition.data(gs).show(gs);
}

std::string SelfTypeParam::toString(const GlobalState &gs, int tabs) {
    return "SelfTypeParam(" + this->definition.data(gs).fullName(gs) + ")";
}

std::string SelfTypeParam::show(const GlobalState &gs) {
    return this->definition.data(gs).show(gs);
}

std::string LambdaParam::typeName() {
    return "LambdaParam";
}

std::string SelfTypeParam::typeName() {
    return "SelfTypeParam";
}
