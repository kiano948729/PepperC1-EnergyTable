#include <vector>
#include <array>

struct Coordinates
{
    int x;
    int y;

    Coordinates(int x, int y) : x(x), y(y) {}
};

struct Point
{
    Coordinates coord;
    int consumption;
    int generation;

    Point(int x, int y, int consumption, int generation) : coord(Coordinates(x, y)), consumption(consumption), generation(generation) {}
};

struct Edge
{
    Point parent;
    std::array<Point, 3> children;
};

int main()
{
    Point hCluster1 = Point(0, 0, 1000, 200);
}