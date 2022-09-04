#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

/*
 * ��������������� ���������, �������� �������� ��� ������ SVG-��������� � ���������.
 * ������ ������ �� ����� ������, ������� �������� � ��� ������� ��� ������ ��������
 */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return { out, indent_step, indent + indent_step };
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

struct Rgb {
    Rgb() = default;

    Rgb(uint8_t r, uint8_t g, uint8_t b)
        : red(r)
        , green(g)
        , blue(b)
    {
    }

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

struct Rgba {
    Rgba() = default;

    Rgba(uint8_t r, uint8_t g, uint8_t b, double o)
        : red(r)
        , green(g)
        , blue(b)
        , opacity(o)
    {
    }

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

struct ColorPrint {
    std::ostream& out;
    void operator()(std::monostate);
    void operator()(std::string s);
    void operator()(Rgb rgb);
    void operator()(Rgba rgba);
};

std::ostream& operator<<(std::ostream& out, const Color& color);

// ������� � ������������ ����� ��������� �� �������������� inline,
// �� ������� ���, ��� ��� ����� ����� �� ��� ������� ����������,
// ������� ���������� ���� ���������.
// � ��������� ������ ������ ������� ���������� ����� ������������ ���� ����� ���� ���������
inline const Color NoneColor{ "none" };

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};
std::ostream& operator<<(std::ostream& out, const StrokeLineCap& stroke_linecap);
std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& stroke_linejoin);

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }
    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_linecap_ = line_cap;
        return AsOwner();
    }
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_linejoin_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_) {
            out << " fill=\""sv << *fill_color_ << "\""sv;
        }
        else {
            out << " fill=\""sv << "none"s << "\""sv;
        }

        if (stroke_color_) {
            out << " stroke=\""sv << *stroke_color_ << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if (stroke_linecap_) {
            out << " stroke-linecap=\""sv << *stroke_linecap_ << "\""sv;
        }
        if (stroke_linejoin_) {
            out << " stroke-linejoin=\""sv << *stroke_linejoin_ << "\""sv;
        }
    }

private:
    Owner& AsOwner() {
        // static_cast ��������� ����������� *this � Owner&,
        // ���� ����� Owner � ��������� PathProps
        return static_cast<Owner&>(*this);
    }
    std::optional<Color> fill_color_; //=nullopt?
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_linecap_;
    std::optional<StrokeLineJoin> stroke_linejoin_;
};

/*
 * ����������� ������� ����� Object ������ ��� ���������������� ��������
 * ���������� ����� SVG-���������
 * ��������� ������� "��������� �����" ��� ������ ����������� ����
 */
class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

/*
 * ����� Circle ���������� ������� <circle> ��� ����������� �����
 *
 */
class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * ����� Polyline ���������� ������� <polyline> ��� ����������� ������� �����
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
    // ��������� ��������� ������� � ������� �����
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;
    std::vector<Point> points_;
    void PointsOut(std::ostream& out) const;//detail?
};

/*
 * ����� Text ���������� ������� <text> ��� ����������� ������
 */
class Text final : public Object, public PathProps<Text> {
public:
    // ����� ���������� ������� ����� (�������� x � y)
    Text& SetPosition(Point pos);

    // ����� �������� ������������ ������� ����� (�������� dx, dy)
    Text& SetOffset(Point offset);

    // ����� ������� ������ (������� font-size)
    Text& SetFontSize(uint32_t size);

    // ����� �������� ������ (������� font-family)
    Text& SetFontFamily(std::string font_family);

    // ����� ������� ������ (������� font-weight)
    Text& SetFontWeight(std::string font_weight);

    // ����� ��������� ���������� ������� (������������ ������ ���� text)
    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;
    Point pos_;
    Point offset_;
    uint32_t size_ = 1;
    std::string font_weight_;
    std::string font_family_;
    std::string data_;
    std::string DataTransformated() const;
};


class ObjectContainer {
public:
    template <typename Obj>
    void Add(Obj obj) {
        objects_.emplace_back(std::make_unique<Obj>(std::move(obj)));
    }
    void virtual AddPtr(std::unique_ptr<Object>&& obj) = 0;
    virtual ~ObjectContainer() = default;
protected:

    std::vector <std::unique_ptr<Object>> objects_;
};

class Drawable {
public:
    virtual void  Draw(ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
};



class Document : public ObjectContainer {
public:

    void AddPtr(std::unique_ptr<Object>&& obj) override;
    // ������� � ostream svg-������������� ���������
    void Render(std::ostream& out) const;

};

}  // namespace svg