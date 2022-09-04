#include "json.h"
#include "json_builder.h"
#include <variant>
#include <memory>
#include <iostream> 
#include <string>

using namespace std::string_literals;

namespace json {

Context::Context(Builder& builder)
    :builder_(builder) {
}
KeyContext& Context::Key(std::string key) {
    return builder_.Key(key);

}
Context& Context::Value(Node::Value value) {
    return builder_.Value(value);
}
StartDictContext& Context::StartDict() {
    return builder_.StartDict();
}
StartArrayContext& Context::StartArray() {
    return builder_.StartArray();

}
Context& Context::EndDict() {
    return builder_.EndDict();
    //return *this;
}
Context& Context::EndArray() {
    return builder_.EndArray();
}
Node Context::Build() {
    return builder_.Build();
}

ValueContext::ValueContext(Builder& builder)
    :Context(builder) {
}

StartDictContext::StartDictContext(Builder& builder)
    :Context(builder) {
}

ValueArrayContext::ValueArrayContext(Builder& builder)
    :Context(builder) {
}

ValueArrayContext& ValueArrayContext::Value(Node::Value value) {
    builder_.Value(value);
    return *this;
}

StartArrayContext::StartArrayContext(Builder& builder)
    :Context(builder) {
}

ValueArrayContext& StartArrayContext::Value(Node::Value value) {
    builder_.Value(value);
    ValueArrayContext* v = new ValueArrayContext(builder_);
    return *v;
}

KeyContext::KeyContext(Builder& builder)
    :Context(builder)
{
}

ValueContext& KeyContext::Value(Node::Value value) {
    builder_.Value(value);
    ValueContext* v = new ValueContext(builder_);
    return *v;
}

Node Builder::CreateNode(Node::Value value) {
    Node* n;
    if (std::holds_alternative<Array>(value)) {
        Array arr = std::get<Array>(value);
        n = new Node(std::move(arr));
    }
    else if (std::holds_alternative<Dict>(value)) {
        Dict di = std::get<Dict>(value);
        n = new Node(std::move(di));
    }
    else if (std::holds_alternative<bool>(value)) {
        bool b = std::get<bool>(value);
        n = new Node(b);
    }
    else if (std::holds_alternative<int>(value)) {
        int i = std::get<int>(value);
        n = new Node(i);
    }
    else if (std::holds_alternative<std::string>(value)) {
        std::string s = std::get<std::string>(value);
        n = new Node(std::move(s));
    }
    else if (std::holds_alternative<double>(value)) {
        double d = std::get<double>(value);
        n = new Node(d);
    }
    else {
        n = new Node();
    }
    return *n;
}

void Builder::AddNode(Node n) {
    if (nodes_stack_.empty()) {
        if (!root_.IsNull()) {
            throw std::logic_error("Root already exists");
        }
        root_ = n;
        return;
    }
    if (!nodes_stack_.back()->IsString() && !nodes_stack_.back()->IsArray()) {
        throw std::logic_error("Attempt to create value failed");
    }
    if (nodes_stack_.back()->IsString()) {
        std::string s = nodes_stack_.back()->AsString();
        nodes_stack_.pop_back();
        std::get<Dict>(nodes_stack_.back()->GetValue()).emplace(std::move(s), n);
        return;
    }
    if (nodes_stack_.back()->IsArray()) {
        std::get<Array>(nodes_stack_.back()->GetValue()).emplace_back(n);
        return;
    }
}

KeyContext& Builder::Key(std::string key) {
    if (nodes_stack_.empty()) {
        throw std::logic_error("Attempt to create a Key outside a Dict");
    }
    Node* n = new Node(std::move(key));
    if (nodes_stack_.back()->IsDict()) {
        nodes_stack_.emplace_back(n);

    }
    KeyContext* c = new KeyContext(*this);
    return *c;
}
Context& Builder::Value(Node::Value value) {
    Node n = CreateNode(value);
    AddNode(n);

    Context* c = new Context(*this);
    return *c;
}

StartDictContext& Builder::StartDict() {
    Node* n = new Node(Dict());
    nodes_stack_.emplace_back(n);
    StartDictContext* c = new StartDictContext(*this);
    return *c;
}

StartArrayContext& Builder::StartArray() {
    Node* n = new Node(Array());
    nodes_stack_.emplace_back(n);
    StartArrayContext* c = new StartArrayContext(*this);
    return *c;
}

Context& Builder::EndDict() {
    Node n = *nodes_stack_.back();
    nodes_stack_.pop_back();
    AddNode(n);
    Context* c = new Context(*this);
    return *c;
}

Context& Builder::EndArray() {
    Node n = *nodes_stack_.back();
    nodes_stack_.pop_back();
    AddNode(n);
    Context* c = new Context(*this);
    return *c;
}

Node Builder::Build() {
    if (root_.IsNull()) {
        throw std::logic_error("Attempt to build an empty json");

    }
    if (!nodes_stack_.empty()) {
        throw std::logic_error("Attempt to build an incomplete json");

    }
    return root_;
}



}//namespace json