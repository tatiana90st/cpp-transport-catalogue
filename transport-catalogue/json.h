#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

class Node;
// —охраните объ€влени€ Dict и Array без изменени€
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Ёта ошибка должна выбрасыватьс€ при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

/*»спользуйте std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>
чтобы хранить текущее значение узла JSON - документа.«начение nullptr имеет тип std::nullptr_t.
ѕоместив nullptr_t в начале списка типов, вы сделаете его типом по умолчанию дл€ этого variant.
ѕоместите этот variant внутрь Node :*/
class Node {
public:
    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;
    Node();
    Node(nullptr_t n);
    Node(Array array);
    Node(Dict map);
    Node(bool b);
    Node(int value);
    Node(double value);
    Node(std::string value);
    // ...
    const Value& GetValue() const { return value_; }

    bool IsInt() const;
    bool IsDouble() const; //¬озвращает true, если в Node хранитс€ int либо double.
    bool IsPureDouble() const; //¬озвращает true, если в Node хранитс€ double.
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    //Ќиже перечислены методы, которые возвращают хран€щеес€ внутри Node значение заданного типа.
    // ≈сли внутри содержитс€ значение другого типа, должно выбрасыватьс€ исключение std::logic_error.
    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const; //.¬озвращает значение типа double, если внутри хранитс€ double либо int.
                             //¬ последнем случае возвращаетс€ приведЄнное в double значение.
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;
private:
    Value value_;
};

bool operator==(const Node& lhs, const Node& rhs);
bool operator!=(const Node& lhs, const Node& rhs);

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

private:
    Node root_;
};

bool operator==(const Document& lhs, const Document& rhs);
bool operator!=(const Document& lhs, const Document& rhs);

Document Load(std::istream& input);

template <typename Value>
void PrintValue(const Value& value, std::ostream& out) {
    out << value;
}

void PrintValue(std::nullptr_t, std::ostream& out);

void PrintValue(const bool b, std::ostream& out);

void PrintValue(const std::string s, std::ostream& out);

void PrintValue(const Array& arr, std::ostream& out);

void PrintValue(const Dict& dict, std::ostream& out);

void PrintNode(const Node& node, std::ostream& out);

void Print(const Document& doc, std::ostream& output);

}  // namespace json