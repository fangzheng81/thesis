#ifndef GRAPH_UTILS_HEADER
#define GRAPH_UTILS_HEADER

#include <cmath>
#include <algorithm>  // std::random_shuffle
#include <utility>    // for std::pair
#include <vector>
#include <chrono>  // for random numbers
#include <ctime>   // for random numbers

#include <ros/ros.h>

#include <octomap_msgs/conversions.h>  // for octomath

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/astar_search.hpp>

namespace drone_coverage
{
// Boost Graph typedefs
typedef std::pair<int, int> Edge;
typedef boost::property<boost::edge_weight_t, double> EdgeWeightProperty;

typedef boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS, boost::no_property, EdgeWeightProperty>
    Graph;
typedef boost::property_map<Graph, boost::edge_weight_t>::type WeightMap;
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_descriptor;
typedef boost::graph_traits<Graph>::edge_descriptor edge_descriptor;

typedef std::pair<double, double> Point_xy;

// Functions
Graph generateGraph(ros::NodeHandle nh, std::vector<Point_xy> points);
std::vector<Point_xy> hillClimbingBase(ros::NodeHandle nh, Graph graph, std::vector<Point_xy> points,
                                       octomap::OcTree* octomap);
std::vector<Point_xy> reorderPoints(std::vector<Point_xy> points, std::vector<int> order);
double calculateCost(Graph graph, std::vector<int> order, std::vector<vertex_descriptor> p, std::vector<double> d);

double getRandomNumber(double i, double j);
double getProbability(double difference, double temperature);
std::vector<int> getNextOrder(std::vector<int> order, int first_index, int second_index);

bool checkIfVisible(const octomap::point3d view_point, const octomap::point3d point_to_test, octomap::OcTree* octomap);

double distanceXY(const Point_xy i, const Point_xy j);

}  // namespace drone_coverage

#endif
