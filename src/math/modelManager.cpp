#include <vector>
#include <array>
#include "modelManager.h"

// defenitions for the coordinates structure defined in modelManager.h
Coordinates::Coordinates(int x, int y) : x(x), y(y) {}
bool Coordinates::operator==(Coordinates other)
{
    if (this->x == other.x && this->y == other.y)
        return true;
    else
        return false;
}
bool Coordinates::operator!=(Coordinates other)
{
    if (this->x != other.x || this->y != other.y)
        return true;
    else
        return false;
}

// defenitions for the point structure defined in modelManager.h
Point::Point(int x, int y, int consumption, int generation) : coord(Coordinates(x, y)), consumption(consumption), generation(generation) {}

// // defenitions for the edge structure defined in modelManager.h
Edge::Edge(Point parent, Point child) : parent(parent), child(child) {}

// utility function not within a struct
int getElectricalFlow(Coordinates startPoint, Coordinates endPoint)
{
}

std::vector<Point> getInbetween(std::vector<Point> allPoints, Coordinates startPoint, Coordinates endPoint)
{
    std::vector<Point> inbetween;

    for (Point tmpPoint : allPoints)
    {
        if (tmpPoint.coord == startPoint)
        {
            continue;
        }

        if (tmpPoint.coord != startPoint && tmpPoint.coord != endPoint)
        {
            inbetween.push_back(tmpPoint);
        }
    }

    return inbetween;
}

int main()
{
    Point hCluster1 = Point(0, 0, 1000, 200);
    Point nfcOne = Point(0, 1, 0, 0);
    Point nfcTwo = Point(0, 2, 0, 0);

    std::vector<Point> points = {
        hCluster1,
        nfcOne};

    std::vector<Point> inbetween = getInbetween(points, Coordinates(0, 0), Coordinates(2, 0));
}