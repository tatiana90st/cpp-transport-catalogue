# TransportCatalogue
Реализация транспортного справочника.
Получение и сохранение данных в формате JSON с использованием собственной библиотеки.
Визуализация карты маршрутов в формате SVG с использованием собственной библиотеки.
Поиск кратчайшего пути по заданным условиям на основе построенной карты маршрутов.
### class TransportCatalogue 
[transport_catalogue.h](https://github.com/tatiana90st/cpp-transport-catalogue/blob/main/transport-catalogue/transport_catalogue.h)  
[transport_catalogue.cpp](https://github.com/tatiana90st/cpp-transport-catalogue/blob/main/transport-catalogue/transport_catalogue.cpp)  
Основные реализованные методы:
- AddStop(const Stop&& stop) - добавление остановки в справочник
- AddBus(const Bus&& bus) - добавление автобусного маршрута в справочник
- FindBus(const std::string_view name) - поиск автобуса по названию
- FindStop(const std::string_view name) - поиск остановки по названию
- GetBusesForStop(Stop* stop) - получения списка проходящих через остановку автобусных маршрутов
- AddDistances(std::pair<std::pair<Stop*, Stop*>, int>&& p) - добавление расстояний между остановками
- GetDistanceBtwStops(Stop* stop1, Stop* stop2) - получение расстояния между остановками
### class TransportRouter
[transport_router.h](https://github.com/tatiana90st/cpp-transport-catalogue/blob/main/transport-catalogue/transport_router.h)  
[transport_router.cpp](https://github.com/tatiana90st/cpp-transport-catalogue/blob/main/transport-catalogue/transport_router.cpp)  
Поиск кратчайшего расстояния между остановками
### class JSONReader 
[json_reader.h](https://github.com/tatiana90st/cpp-transport-catalogue/blob/main/transport-catalogue/json_reader.h)  
[json_reader.cpp](https://github.com/tatiana90st/cpp-transport-catalogue/blob/main/transport-catalogue/json_reader.cpp)  
Чтение из потока ввода запросов на построение базы данных транспортного справочника, чтение запросов к транспортному справочнику, формирование ответов на полученные запросы в формате JSON
### class MapRenderer
[map_renderer.h](https://github.com/tatiana90st/cpp-transport-catalogue/blob/main/transport-catalogue/map_renderer.h)  
[map_renderer.cpp](https://github.com/tatiana90st/cpp-transport-catalogue/blob/main/transport-catalogue/map_renderer.cpp)  
Визуализация карты маршрутов в формате SVG
<!-- ДОБАВИТЬ ПРИМЕРЫ-->
<!-- ДОБАВИТЬ ТЕСТЫ-->
<!-- ДОБАВИТЬ КАРТИНКИ-->
<!-- Не помешало бы доработать визуализацию кратчайшего пути-->
