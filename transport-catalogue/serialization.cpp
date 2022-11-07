#include "transport_catalogue.h"
#include "serialization.h"
#include "domain.h"
#include <string>
#include <transport_catalogue.pb.h>
#include <fstream>
#include <variant>
#include <map>

namespace serial {

Serializator::Serializator(tc_serialize::TransportBase db)
	:db_(std::move(db)) {

}

void SerializeTrCatalogue(std::ofstream& out_file, const RequestHandler& rh, const RenderSettings& rend_set, const TransportRouter& tr_router) {
	Serializator serializator;
	serializator.BuildStops(rh.GetStopsIndex());
	serializator.BuildBuses(rh.GetAllBuses());
	serializator.BuildDistances(rh.GetDistances());
	serializator.BuildRenderSettings(rend_set);
	serializator.BuildRouter(tr_router);
	serializator.SaveBaseToFile(out_file);
}

void Serializator::BuildStops(const std::map<std::string_view, Stop*>& stops) {
	int i = 0;
	for (const auto& [name, ptr] : stops) {
		tc_serialize::Stop* stop = db_.add_stops();
		stop->set_name(std::string(name));
		stop->set_id(i);
		stops_map_[std::string(name)] = i;
		stop->set_lat(ptr->place.lat);
		stop->set_lng(ptr->place.lng);
		++i;
	}
}

void Serializator::BuildDistances(const std::unordered_map<std::pair<Stop*, Stop*>, int, tr_cat::detail::PairPtrHasher>& distances) {
	for (const auto& [stops, dist] : distances) {
		tc_serialize::Distance* dist_ser = db_.add_distances();
		dist_ser->set_first_stop_id(stops_map_.at(stops.first->name));
		dist_ser->set_second_stop_id(stops_map_.at(stops.second->name));
		dist_ser->set_distance(dist);
	}
}

void Serializator::BuildBuses(const std::map<std::string_view, Bus*> buses) {
	int j = 0;
	for (const auto& [name, ptr] : buses) {
		tc_serialize::Bus* bus = db_.add_buses();
		bus->set_name(std::string(name));
		bus->set_is_round(ptr->is_round);
		bus->set_id(j);
		buses_map_[std::string(name)] = j;
		if (!ptr->route.empty()) {
			for (int i = 0; i < ptr->route.size(); ++i) {
				Stop* stop_ptr = ptr->route[i];
				if (stops_map_.count(stop_ptr->name)) {
					int stop_id = stops_map_.at(stop_ptr->name);
					bus->add_route(stop_id);
					if (!ptr->half_route.empty() && i < ptr->half_route.size()) {
						bus->add_half_route(stop_id);
					}
				}
			}
		}
		++j;
	}
	
}

void Serializator::BuildRenderSettings(const RenderSettings& rend_set) {
	tc_serialize::RenderSettings* settings = db_.mutable_rend_set();
	
	settings->set_width(rend_set.width);
	settings->set_height(rend_set.height);
	settings->set_padding(rend_set.padding);
	settings->set_line_width(rend_set.line_width);
	settings->set_stop_radius(rend_set.stop_radius);
	settings->set_bus_label_font_size(rend_set.bus_label_font_size);
	settings->set_stop_label_font_size(rend_set.stop_label_font_size);
	settings->set_underlayer_width(rend_set.underlayer_width);
	
	settings->mutable_bus_label_offset()->set_x(rend_set.bus_label_offset.x);
	settings->mutable_bus_label_offset()->set_y(rend_set.bus_label_offset.y);
	settings->mutable_stop_label_offset()->set_x(rend_set.stop_label_offset.x);
	settings->mutable_stop_label_offset()->set_y(rend_set.stop_label_offset.y);
	
	tc_serialize::Color c = FormatColor(rend_set.underlayer_color);
	*settings->mutable_underlayer_color() = c;

	for (const svg::Color& svg_color : rend_set.color_palette) {
		tc_serialize::Color* serial_color = settings->add_color_palette();
		tc_serialize::Color formated_svg_color = FormatColor(svg_color);
		*serial_color = formated_svg_color;
	}
}

void Serializator::BuildRouter(const TransportRouter& tr_router) {
	*db_.mutable_graph() = tr_router.GetSerializedGraph();
	*db_.mutable_router() = tr_router.GetSerializedRouter();
	*db_.mutable_transport_router() = tr_router.GetSerializedTransportRouter(/*stops_map_, buses_map_*/);
}

void Serializator::SaveBaseToFile(std::ofstream& out_file) {
	db_.SerializeToOstream(&out_file);
}

void Serializator::BuildCatalogue(tr_cat::TransportCatalogue& tr_cat) {
	AddStops(tr_cat);
	AddDistances(tr_cat);
	AddBuses(tr_cat);
}

void Serializator::AddSettings(RenderSettings& rend_set) {
	rend_set.width = db_.rend_set().width();
	rend_set.height = db_.rend_set().height();
	rend_set.padding = db_.rend_set().padding();
	rend_set.stop_radius = db_.rend_set().stop_radius();
	rend_set.line_width = db_.rend_set().line_width();
	rend_set.bus_label_font_size = db_.rend_set().bus_label_font_size();
	rend_set.stop_label_font_size = db_.rend_set().stop_label_font_size();
	rend_set.underlayer_width = db_.rend_set().underlayer_width();
	rend_set.bus_label_offset.x = db_.rend_set().bus_label_offset().x();
	rend_set.bus_label_offset.y = db_.rend_set().bus_label_offset().y();
	rend_set.stop_label_offset.x = db_.rend_set().stop_label_offset().x();
	rend_set.stop_label_offset.y = db_.rend_set().stop_label_offset().y();
	rend_set.underlayer_color = TransformColorToSvg(db_.rend_set().underlayer_color());

	rend_set.color_palette.resize(db_.rend_set().color_palette_size());
	for (int i = 0; i < db_.rend_set().color_palette_size(); ++i) {
		rend_set.color_palette[i] = TransformColorToSvg(db_.rend_set().color_palette(i));
	}
}

void Serializator::SetRouterSettings(RouterSettings& r_set) {
	r_set.bus_velocity_ = db_.transport_router().rout_set().bus_velocity();
	r_set.bus_wait_time_ = db_.transport_router().rout_set().bus_wait_time();
}

TransportRouter* DeserializeTrCatalogue(std::ifstream& in, tr_cat::TransportCatalogue& tr_cat, RenderSettings& rend_set, RouterSettings& r_set, RequestHandler& rh) {
	tc_serialize::TransportBase base;
	bool parsed = base.ParseFromIstream(&in);
	Serializator serializator(base);
	serializator.BuildCatalogue(tr_cat);
	serializator.AddSettings(rend_set);
	graph::DirectedWeightedGraph<double>* graph = serializator.EctractGraph();
	graph::Router<double>* router = serializator.ExtractRouter(*graph);
	serializator.SetRouterSettings(r_set);
	TransportRouter* tr_router = serializator.ExtractTrRouter(tr_cat, r_set, graph, router, rh);
	return tr_router;
}

TransportRouter* Serializator::ExtractTrRouter(tr_cat::TransportCatalogue& tr_cat, RouterSettings r_set,
	graph::DirectedWeightedGraph<double>* graph, graph::Router<double>* router, RequestHandler& rh) {

	TransportRouter* tr_router = new TransportRouter(tr_cat, r_set, rh, graph, router);
	return tr_router;
}


graph::Router<double>* Serializator::ExtractRouter(const graph::DirectedWeightedGraph<double>& graph) {

	graph::RoutesInternalData<double> madness(db_.router().routes_internal_data_size());
	for (int i = 0; i < madness.size(); ++i) {
		madness[i].resize(db_.router().routes_internal_data(i).vector_route_size());
		for (int j = 0; j < madness[i].size(); ++j) {
			//deal with this later
			if (db_.router().routes_internal_data(i).vector_route(j).has_v() == false) {
				madness[i][j] = std::nullopt;
			}
			else {
				graph::RouteInternalData<double> something;
				something.weight = db_.router().routes_internal_data(i).vector_route(j).weight();
				if (db_.router().routes_internal_data(i).vector_route(j).has_prev_edge()) {
					something.prev_edge = db_.router().routes_internal_data(i).vector_route(j).prev_edge().prev_edge_id();
				}
				else {
					something.prev_edge = std::nullopt;
				}
				madness[i][j] = something;
			}
		}
	}
	graph::Router<double>* router = new graph::Router<double>(graph, madness);
	return router;
}

graph::DirectedWeightedGraph<double>* Serializator::EctractGraph() {

	std::vector<graph::Edge<double>> edges(db_.graph().edges_size());
	for (int i = 0; i < db_.graph().edges_size(); ++i) {
		graph::Edge<double> edge;
		edge.weight = db_.graph().edges(i).weight();
		edge.from = db_.graph().edges(i).vertex_from();
		edge.to = db_.graph().edges(i).vertex_to();
		edges[i] = edge;
	}

	std::vector<std::vector<graph::EdgeId>> incidence_lists(db_.graph().incidence_lists_size());
	for (int i = 0; i < db_.graph().incidence_lists_size(); ++i) {
		incidence_lists[i].resize(db_.graph().incidence_lists(i).edge_id_incidence_list_size());
		for (int j = 0; j < db_.graph().incidence_lists(i).edge_id_incidence_list_size(); ++j) {
			incidence_lists[i][j] = db_.graph().incidence_lists(i).edge_id_incidence_list(j);
		}
	}
	graph::DirectedWeightedGraph<double>* graph = new graph::DirectedWeightedGraph<double>(edges, incidence_lists);
	return graph;
}

void Serializator::AddStops(tr_cat::TransportCatalogue& tr_cat) {
	for (int i = 0; i < db_.stops_size(); ++i) {
		Stop stop = domain::MakeStop(db_.stops(i).name(), db_.stops(i).lat(), db_.stops(i).lng());
		Stop* stop_ptr = tr_cat.AddStop(std::move(stop));
		id_to_stop_[db_.stops(i).id()] = stop_ptr;
	}
}

void Serializator::AddDistances(tr_cat::TransportCatalogue& tr_cat) {
	for (int i = 0; i < db_.distances_size(); ++i) {
		Stop* stop1 = nullptr;
		if (id_to_stop_.count(db_.distances(i).first_stop_id())) {
			stop1 = id_to_stop_.at(db_.distances(i).first_stop_id());
		}
		Stop* stop2 = nullptr;
		if (id_to_stop_.count(db_.distances(i).second_stop_id())) {
			stop2 = id_to_stop_.at(db_.distances(i).second_stop_id());
		}
		tr_cat.AddDistances(std::make_pair((std::make_pair(stop1, stop2)), db_.distances(i).distance()));
	}
}

void Serializator::AddBuses(tr_cat::TransportCatalogue& tr_cat) {
	for (int i = 0; i < db_.buses_size(); ++i) {
		Bus bus;
		bus.name = db_.buses(i).name();
		bus.is_round = db_.buses(i).is_round();
		bus.route.resize(db_.buses(i).route_size());
		bus.half_route.resize(db_.buses(i).half_route_size());
		for (int j = 0; j < db_.buses(i).route_size(); ++j) {
			bus.route[j] = id_to_stop_.count(db_.buses(i).route(j)) ? id_to_stop_.at(db_.buses(i).route(j)) : nullptr;
			bus.unique_stops.insert(bus.route[j]);
			if (!bus.half_route.empty() && j < bus.half_route.size()) {
				bus.half_route[j] = bus.route[j];
			}
		}
		Bus* bus_ptr_ = tr_cat.AddBus(std::move(bus));
		id_to_bus_[db_.buses(i).id()] = bus_ptr_;
	}
}

tc_serialize::Color FormatColor(svg::Color svg_color) {
	tc_serialize::Color color;

	if (std::holds_alternative<std::monostate>(svg_color)) {
		color.set_monostate(true);
	}
	else if (std::holds_alternative<std::string>(svg_color)) {
		
		color.set_string_color(std::get<std::string>(svg_color));
	}
	else if (std::holds_alternative<svg::Rgb>(svg_color)) {
		
		svg::Rgb c = std::get<svg::Rgb>(svg_color);
		color.mutable_rgb_color()->set_red(c.red);
		color.mutable_rgb_color()->set_green(c.green);
		color.mutable_rgb_color()->set_blue(c.blue);
	}
	else if (std::holds_alternative<svg::Rgba>(svg_color)) {
		
		svg::Rgba c = std::get<svg::Rgba>(svg_color);
		color.mutable_rgba_color()->set_red(c.red);
		color.mutable_rgba_color()->set_green(c.green);
		color.mutable_rgba_color()->set_blue(c.blue);
		color.mutable_rgba_color()->set_opacity(c.opacity);
	}
	return color;

}

svg::Color TransformColorToSvg(const tc_serialize::Color& serial_color) {
	svg::Color svg_color;
    
	if(serial_color.variant_colors_case() == tc_serialize::Color::VariantColorsCase::kStringColor){
		svg_color = serial_color.string_color();
	}
	else if (serial_color.variant_colors_case() == tc_serialize::Color::VariantColorsCase::kRgbColor) {
		svg::Rgb rgb;
		rgb.red = serial_color.rgb_color().red();
		rgb.green = serial_color.rgb_color().green();
		rgb.blue = serial_color.rgb_color().blue();
		svg_color = rgb;
	}
	else if (serial_color.variant_colors_case() == tc_serialize::Color::VariantColorsCase::kRgbaColor) {
		svg::Rgba rgba;
		rgba.red = serial_color.rgba_color().red();
		rgba.green = serial_color.rgba_color().green();
		rgba.blue = serial_color.rgba_color().blue();
		rgba.opacity = serial_color.rgba_color().opacity();
		svg_color = rgba;
	}

	return svg_color;
}


}//namespace serial