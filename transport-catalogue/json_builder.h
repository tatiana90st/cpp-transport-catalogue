#pragma once
#include "json.h"
#include <variant>
#include <memory>
#include <iostream> 
#include <string>

using namespace std::string_literals;

namespace json {

class Builder;
class KeyContext;
class StartDictContext;
class StartArrayContext;

class Context {
protected:
    Builder& builder_;

public:
    Context(Builder& builder);
    KeyContext& Key(const std::string& key);
    Context& Value(Node::Value value);
    StartDictContext& StartDict();
    StartArrayContext& StartArray();
    Context& EndDict();
    Context& EndArray();
    Node Build();
};

class ValueContext : public Context {
public:
    ValueContext(Builder& builder);

    Context& Value(Node::Value value) = delete;
    StartDictContext& StartDict() = delete;
    StartArrayContext& StartArray() = delete;
    Context& EndArray() = delete;
    Node Build() = delete;
};

class StartDictContext : public Context {
public:
    StartDictContext(Builder& builder);
    Context& Value(Node::Value value) = delete;
    StartDictContext& StartDict() = delete;
    StartArrayContext& StartArray() = delete;
    Context& EndArray() = delete;
    Node Build() = delete;
};

class ValueArrayContext : public Context {
public:
    ValueArrayContext(Builder& builder);
    ValueArrayContext& Value(Node::Value value);
    KeyContext& Key(const std::string& key) = delete;
    Context& EndDict() = delete;
    Node Build() = delete;
};

class StartArrayContext :public Context {
public:
    StartArrayContext(Builder& builder);
    KeyContext& Key(const std::string& key) = delete;
    ValueArrayContext& Value(Node::Value value);
    Context& EndDict() = delete;
    Node Build() = delete;
};

class KeyContext : public Context {
public:
    KeyContext(Builder& builder);
    ValueContext& Value(Node::Value value);
    KeyContext& Key(const std::string& key) = delete;
    Context& EndDict() = delete;
    Context& EndArray() = delete;
    Node Build() = delete;
};

class Builder {

private:
    Node root_;

    std::vector<std::unique_ptr<Node>>nodes_stack_;

    Node CreateNode(Node::Value value);

    void AddNode(Node n);

public:
    KeyContext& Key(const std::string& key);
    Context& Value(Node::Value value);

    StartDictContext& StartDict();

    StartArrayContext& StartArray();

    Context& EndDict();

    Context& EndArray();

    Node Build();
};


}//namespace json