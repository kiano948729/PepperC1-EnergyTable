#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include <array>

struct Coordinates
{
    int x;
    int y;

    Coordinates(int x, int y);

    bool operator==(Coordinates other) {};

    bool operator!=(Coordinates other) {};
};

struct Point
{
    Coordinates coord;
    std::vector<Edge> connected;

    int consumption;
    int generation;

    Point(int x, int y, int consumption, int generation);
    std::vector<Coordinates> getConnectedCoords();
};

struct Edge
{
    Point parent;
    Point child;

    Edge(Point parent, Point child);
};

int getElectricalFlow(Coordinates startPoint, Coordinates endPoint);

std::vector<Edge> griddy(std::vector<Point> points);

#endif