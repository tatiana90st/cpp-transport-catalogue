#include "json.h"
#include <variant>
#include <stdexcept>
#include <sstream>
#include <algorithm>

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;

    for (char c; input >> c && c != ']';) {

        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));

        if (input >> c) {
            if (c == ']') {
                break;
            }
            if (c != ',') {
                throw ParsingError("Invalid array");
            }
        }
        else {
            throw ParsingError("Invalid array");
        }
    }
    if (!input) {
        throw ParsingError("Invalid array");
    }
    return Node(move(result));
}

using Number = std::variant<int, double>;

Number LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    }
    else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            }
            catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    }
    catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadNull(istream& input) {
    string s;
    while (std::isalpha(static_cast<unsigned char>(input.peek()))) {
        s.push_back(static_cast<unsigned char>(input.get()));
    }

    if (s == "null"s) {
        return Node(nullptr);
    }
    throw ParsingError("Parsing error");

}

Node LoadBool(istream& input) {
    string s;
    while (std::isalpha(static_cast<unsigned char>(input.peek()))) {
        s.push_back(static_cast<unsigned char>(input.get()));
    }
    if (s == "false"s) {
        return Node(false);
    }
    if (s == "true"s) {
        return Node(true);
    }
    else {
        throw ParsingError("Boolean parsing error");
    }
}

Node LoadString(istream& input) {
    using namespace std::literals;

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        }
        // Пробелы, табуляции и символы перевода строки между токенами JSON файла игнорируются 
        //assert(LoadJSON("\t\r\n\n\r \"Hello\" \t\r\n\n\r ").GetRoot() == Node{ "Hello"s });
        else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
            case 'n':
                s.push_back('\n');
                break;
            case 't':
                s.push_back('\t');
                break;
            case 'r':
                s.push_back('\r');
                break;
            case '"':
                s.push_back('"');
                break;
            case '\\':
                s.push_back('\\');
                break;
            default:
                // Встретили неизвестную escape-последовательность
                throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        }
        else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        }
        else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }
    return s;
}

Node LoadDict(istream& input) {
    Dict result;
    for (char c; input >> c && c != '}';) {
        if (c == '"') {
            string key = LoadString(input).AsString();
            if (result.count(key)) {
                throw ParsingError("Map parsing error: keys should be different");
            }
            if (input >> c && c != ':') {
                throw ParsingError("Map parsing error: incorrect format");
            }
            result.insert({ move(key), LoadNode(input) });
        }
        else if (c != ',') {
            throw ParsingError("Map parsing error: incorrect format");
        }

    }
    if (!input) {
        throw ParsingError("Invalid map");
    }
    return Node(move(result));
}

Node LoadNode(istream& input) {
    char c;
    input >> c;
    if (c == '[') {
        return LoadArray(input);//проверить как там с bool и null
    }
    else if (c == '{') {
        return LoadDict(input);//проверить
    }
    else if (c == '"') {
        return LoadString(input);
    }
    else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    }
    else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    }
    else {
        input.putback(c);
        Number n = LoadNumber(input);
        if (holds_alternative<double>(n)) {
            return Node(get<double>(n));
        }
        else {
            return Node(get<int>(n));
        }
    }
}

}  // namespace
Node::Node()
    :value_(nullptr) {
}
Node::Node(nullptr_t)
    :value_(nullptr) {
}

Node::Node(Array array)
    : value_(move(array)) {
}

Node::Node(Dict map)
    : value_(move(map)) {
}

Node::Node(bool b)
    :value_(b) {
}
Node::Node(int value)
    : value_(value) {
}
Node::Node(double value)
    : value_(value) {
}
Node::Node(string value)
    : value_(move(value)) {
}

bool Node::IsInt() const {
    return holds_alternative<int>(value_);
}
bool Node::IsDouble() const {
    return (holds_alternative<int>(value_) || holds_alternative<double>(value_));
}
bool Node::IsPureDouble() const {
    return holds_alternative<double>(value_);
}
bool Node::IsBool() const {
    return holds_alternative<bool>(value_);
}
bool Node::IsString() const {
    return holds_alternative<string>(value_);
}
bool Node::IsNull() const {
    return holds_alternative<nullptr_t>(value_);
}
bool Node::IsArray() const {
    return holds_alternative<Array>(value_);
}
bool Node::IsMap() const {
    return holds_alternative<Dict>(value_);
}

const Array& Node::AsArray() const {
    if (const auto* value = get_if<Array>(&value_)) {
        return *value;
    }
    else {
        throw logic_error("attempt to get an array failed");
    }
}

const Dict& Node::AsMap() const {
    if (const auto* value = get_if<Dict>(&value_)) {
        return *value;
    }
    else {
        throw logic_error("attempt to get a map failed");
    }
}

int Node::AsInt() const {
    if (const auto* value = get_if<int>(&value_)) {
        return *value;
    }
    else {
        throw logic_error("attempt to get an int-number failed");
    }
}

bool Node::AsBool() const {
    if (const auto* value = get_if<bool>(&value_)) {
        return *value;
    }
    else {
        throw logic_error("attempt to get boolean failed");
    }
}

double Node::AsDouble() const {
    if (const auto* value = get_if<double>(&value_)) {
        return *value;
    }
    else if (const auto* value = get_if<int>(&value_)) {
        return static_cast<double>(*value);
    }
    else {
        throw logic_error("attempt to get a float-number failed");
    }
}

const string& Node::AsString() const {
    if (const auto* value = get_if<string>(&value_)) {
        return *value;
    }
    else {
        throw logic_error("attempt to get a string failed");
    }
}

bool operator==(const Node& lhs, const Node& rhs) {
    return lhs.GetValue() == rhs.GetValue();
}
bool operator!=(const Node& lhs, const Node& rhs) {
    return lhs.GetValue() != rhs.GetValue();
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool operator==(const Document& lhs, const Document& rhs) {
    return(lhs.GetRoot() == rhs.GetRoot());
}
bool operator!=(const Document& lhs, const Document& rhs) {
    return(lhs.GetRoot() != rhs.GetRoot());
}

Document Load(istream& input) {
    return Document{ LoadNode(input) };
}


// Перегрузка функции PrintValue для вывода значений null
void PrintValue(std::nullptr_t, std::ostream& out) {
    out << "null"sv;
}

void PrintValue(const bool b, std::ostream& out) {
    out << boolalpha << b;
}

std::string Unparsed(const std::string s) {
    string p = s;
    size_t counter = 0;
    size_t i = 0;
    for (const char l : s) {
        if (l == (char)92) {
            p.replace(i + counter, 1, "\\\\");
            ++counter;
        }
        if (l == (char)34) {
            p.replace(i + counter, 1, "\\\"");
            ++counter;
        }

        if (l == '\n') {
            p.replace(i + counter, 1, "\\n");
            ++counter;
        }
        if (l == '\r') {
            p.replace(i + counter, 1, "\\r");
            ++counter;
        }
        ++i;
    }
    return p;
}

void PrintValue(const std::string s, std::ostream& out) {
    out << "\"" << Unparsed(s) << "\"";
}

void PrintValue(const Array& arr, std::ostream& out) {
    out << "["s;
    bool start = true;
    for (size_t i = 0; i < arr.size(); ++i) {
        if (start) {
            PrintNode(arr[i], out);
            start = false;
        }
        else {
            out << ","s;
            PrintNode(arr[i], out);
        }
    }
    out << "]"s;
}

void PrintValue(const Dict& dict, std::ostream& out) {
    out << "{"s;
    bool start = true;
    for (const auto& d : dict) {
        if (start) {
            PrintNode(d.first, out);
            out << ":";
            PrintNode(d.second, out);
            start = false;
        }
        else {
            out << ","s;
            PrintNode(d.first, out);
            out << ":";
            PrintNode(d.second, out);
        }
    }
    out << "}"s;
}

void PrintNode(const Node& node, std::ostream& out) {
    std::visit(
        [&out](const auto& value) { PrintValue(value, out); },
        node.GetValue());
}

void Print(const Document& doc, std::ostream& output) {

    PrintNode(doc.GetRoot(), output);

}

}  // namespace json