#include "domain.h"

namespace domain{
Stop MakeStop(std::string name, double lat, double lng) {
	Stop stop;
	stop.name = name;
	stop.place.lat = lat;
	stop.place.lng = lng;
	return stop;
}

}
