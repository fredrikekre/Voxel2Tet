#include <algorithm>

#include "MeshComponents.h"

namespace voxel2tet
{
// VertexType

VertexType :: VertexType(double x, double y, double z)
{
    this->c [ 0 ] = x;
    this->c [ 1 ] = y;
    this->c [ 2 ] = z;
    this->originalcoordinates [ 0 ] = x;
    this->originalcoordinates [ 1 ] = y;
    this->originalcoordinates [ 2 ] = z;
    this->Fixed = { { false, false, false } };
    this->c_constant = 0.0;
    this->error = 0.0;
}

void VertexType :: set_c(std :: array< double, 3 >newc)
{
    this->c = newc;
    for ( TriangleType *t : this->Triangles ) {
        t->UpdateNormal();
    }
}

void VertexType :: set_c(double c, int index)
{
    this->c [ index ] = c;
    for ( TriangleType *t : this->Triangles ) {
        t->UpdateNormal();
    }
}

std :: array< double, 3 >VertexType :: get_c()
{
    return this->c;
}

double VertexType :: get_c(int index)
{
    return this->c [ index ];
}

void VertexType :: AddPhaseEdge(PhaseEdge *pe)
{
    this->PhaseEdges.push_back(pe);
    std :: sort( this->PhaseEdges.begin(), this->PhaseEdges.end() );
    this->PhaseEdges.erase( std :: unique( this->PhaseEdges.begin(), this->PhaseEdges.end() ), this->PhaseEdges.end() );
}

void VertexType :: AddTriangle(TriangleType *Triangle)
{
    for ( TriangleType *T : this->Triangles ) {
        if ( T == Triangle ) {
            return;
        }
    }
    this->Triangles.push_back(Triangle);
}

void VertexType :: RemoveTriangle(TriangleType *Triangle)
{
    this->Triangles.erase( std :: remove(this->Triangles.begin(), this->Triangles.end(), Triangle), this->Triangles.end() );
}

void VertexType :: AddEdge(EdgeType *Edge)
{
    for ( EdgeType *E : this->Edges ) {
        if ( E == Edge ) {
            return;
        }
    }
    this->Edges.push_back(Edge);
}

void VertexType :: RemoveEdge(EdgeType *Edge)
{
    this->Edges.erase( std :: remove(this->Edges.begin(), this->Edges.end(), Edge), this->Edges.end() );
}

std :: vector< VertexType * >VertexType :: FetchNeighbouringVertices()
{
    std :: vector< VertexType * >Neighbours;

    for ( auto e : this->Edges ) {
        for ( auto v : e->Vertices ) {
            if ( v != this ) {
                Neighbours.push_back(v);
            }
        }
    }
    return Neighbours;
}

// EdgeType

std :: vector< TriangleType * >EdgeType :: GiveTriangles()
{
    std :: vector< TriangleType * >TriangleCollection;

    for ( int i : { 0, 1 } ) {
        std :: sort( this->Vertices [ i ]->Triangles.begin(), this->Vertices [ i ]->Triangles.end() );
    }

    std :: set_intersection( this->Vertices [ 0 ]->Triangles.begin(), this->Vertices [ 0 ]->Triangles.end(),
                             this->Vertices [ 1 ]->Triangles.begin(), this->Vertices [ 1 ]->Triangles.end(), std :: back_inserter(TriangleCollection) );

    return TriangleCollection;
}

double EdgeType :: GiveLength()
{
    std :: array< double, 3 >v;
    for ( int i = 0; i < 3; i++ ) {
        v [ i ] = this->Vertices [ 1 ]->get_c(i) - this->Vertices [ 0 ]->get_c(i);
    }

    return std :: sqrt(v [ 0 ] * v [ 0 ] + v [ 1 ] * v [ 1 ] + v [ 2 ] * v [ 2 ]);
}

std :: array< double, 3 >EdgeType :: GiveCenterPoint()
{
    double x = ( this->Vertices [ 0 ]->get_c(0) + this->Vertices [ 1 ]->get_c(0) ) / 2.0;
    double y = ( this->Vertices [ 0 ]->get_c(1) + this->Vertices [ 1 ]->get_c(1) ) / 2.0;
    double z = ( this->Vertices [ 0 ]->get_c(2) + this->Vertices [ 1 ]->get_c(2) ) / 2.0;
    return { {
                 x, y, z
             } };
}

// TriangleType

TriangleType :: TriangleType(std :: array< VertexType *, 3 >Vertices)
{
    this->Vertices = Vertices;
    this->UpdateNormal();
}

std :: array< double, 3 >TriangleType :: GiveEdgeVector(int node)
{
    std :: array< double, 3 >edgevector;

    int nextnode = ( node < 2 ) ? ( node + 1 ) : ( 0 );

    for ( int i = 0; i < 3; i++ ) {
        edgevector [ i ] = this->Vertices [ nextnode ]->get_c(i) - this->Vertices [ node ]->get_c(i);
    }

    return edgevector;
}

std :: array< double, 3 >TriangleType :: GiveUnitNormal()
{
    double l = std :: sqrt(this->Normal [ 0 ] * this->Normal [ 0 ] + this->Normal [ 1 ] * this->Normal [ 1 ] + this->Normal [ 2 ] * this->Normal [ 2 ]);
    std :: array< double, 3 >NormalizedNormal = this->GiveNormal();
    for ( int i = 0; i < 3; i++ ) {
        NormalizedNormal [ i ] = NormalizedNormal [ i ] / l;
    }
    return NormalizedNormal;
}

double TriangleType :: GiveArea() // TODO: Should call GiveSignedArea
{
    double e1 [ 3 ], e2 [ 3 ], n [ 3 ];

    // Compute two vectors with origin in vertex 0 describing the triangle
    for ( int i = 0; i < 3; i++ ) {
        e1 [ i ] = this->Vertices [ 1 ]->get_c(i) - this->Vertices [ 0 ]->get_c(i);
        e2 [ i ] = this->Vertices [ 2 ]->get_c(i) - this->Vertices [ 0 ]->get_c(i);
    }

    // Compute cross product of the two vectors
    n [ 0 ] = e1 [ 1 ] * e2 [ 2 ] - e1 [ 2 ] * e2 [ 1 ];
    n [ 1 ] = -e1 [ 0 ] * e2 [ 2 ] + e2 [ 0 ] * e1 [ 2 ];
    n [ 2 ] = e1 [ 0 ] * e2 [ 1 ] - e2 [ 0 ] * e1 [ 1 ];

    // Compute area
    double Area = std :: sqrt(n [ 0 ] * n [ 0 ] + n [ 1 ] * n [ 1 ] + n [ 2 ] * n [ 2 ]) / 2.0;
    return Area;
}

double TriangleType :: GiveSignedArea()
{
    double e1 [ 3 ], e2 [ 3 ], n [ 3 ];

    // Compute two vectors with origin in vertex 0 describing the triangle
    for ( int i = 0; i < 3; i++ ) {
        e1 [ i ] = this->Vertices [ 1 ]->get_c(i) - this->Vertices [ 0 ]->get_c(i);
        e2 [ i ] = this->Vertices [ 2 ]->get_c(i) - this->Vertices [ 0 ]->get_c(i);
    }

    // Compute cross product of the two vectors
    n [ 0 ] = e1 [ 1 ] * e2 [ 2 ] - e1 [ 2 ] * e2 [ 1 ];
    n [ 1 ] = -e1 [ 0 ] * e2 [ 2 ] + e2 [ 0 ] * e1 [ 2 ];
    n [ 2 ] = e1 [ 0 ] * e2 [ 1 ] - e2 [ 0 ] * e1 [ 1 ];

    // Compute area
    double t = n [ 0 ] * n [ 0 ] + n [ 1 ] * n [ 1 ] + n [ 2 ] * n [ 2 ];
    double Area = std :: sqrt(t) / 2.0 * t / fabs(t);
    return Area;
}

std :: array< double, 3 >TriangleType :: GiveCenterOfMass()
{
    std :: array< double, 3 >cm = { { 0, 0, 0 } };
    for ( int i = 0; i < 3; i++ ) {
        for ( int j = 0; j < 3; j++ ) {
            cm [ i ] = cm [ i ] + this->Vertices [ j ]->get_c(i) / 3;
        }
    }
    return cm;
}

double TriangleType :: GiveLargestAngle(int *index)
{
    std :: array< std :: array< double, 3 >, 3 >e;
    std :: array< double, 3 >length;
    std :: array< double, 3 >alpha;

    for ( int i = 0; i < 3; i++ ) {
        e [ i ] = this->GiveEdgeVector(i);
        length [ i ] = std :: sqrt(e [ i ] [ 0 ] * e [ i ] [ 0 ] + e [ i ] [ 1 ] * e [ i ] [ 1 ] + e [ i ] [ 2 ] * e [ i ] [ 2 ]);
    }

    double LargestAngle = -1.0;

    for ( int node = 0; node < 3; node++ ) {
        int prevnode = ( node > 0 ) ? ( node - 1 ) : ( 2 );

        alpha [ node ] = std :: acos( -( e [ node ] [ 0 ] * e [ prevnode ] [ 0 ] + e [ node ] [ 1 ] * e [ prevnode ] [ 1 ] + e [ node ] [ 2 ] * e [ prevnode ] [ 2 ] ) / ( length [ node ] * length [ prevnode ] ) );
        if ( alpha [ node ] > LargestAngle ) {
            LargestAngle = alpha [ node ];
            if ( index != NULL ) {
                * index = node;
            }
        }
    }

    return LargestAngle;
}

double TriangleType :: GiveSmallestAngle(int *index)
{
    std :: array< std :: array< double, 3 >, 3 >e;
    std :: array< double, 3 >length;
    std :: array< double, 3 >alpha;

    for ( int i = 0; i < 3; i++ ) {
        e [ i ] = this->GiveEdgeVector(i);
        length [ i ] = std :: sqrt(e [ i ] [ 0 ] * e [ i ] [ 0 ] + e [ i ] [ 1 ] * e [ i ] [ 1 ] + e [ i ] [ 2 ] * e [ i ] [ 2 ]);
    }

    double SmallestAngle = 100.0;

    for ( int node = 0; node < 3; node++ ) {
        int prevnode = ( node > 0 ) ? ( node - 1 ) : ( 2 );

        alpha [ node ] = std :: acos( -( e [ node ] [ 0 ] * e [ prevnode ] [ 0 ] + e [ node ] [ 1 ] * e [ prevnode ] [ 1 ] + e [ node ] [ 2 ] * e [ prevnode ] [ 2 ] ) / ( length [ node ] * length [ prevnode ] ) );
        if ( alpha [ node ] < SmallestAngle ) {
            SmallestAngle = alpha [ node ];
            if ( index != NULL ) {
                * index = node;
            }
        }
    }

    return SmallestAngle;
}

EdgeType *TriangleType :: GiveEdge(int Index)
{
    std :: array< EdgeType *, 3 >Edges = this->GiveEdges();
    return Edges.at(Index);
}

std :: array< EdgeType *, 3 >TriangleType :: GiveEdges()
{
    std :: vector< EdgeType * >EdgeCollection;

    for ( int i = 0; i < 3; i++ ) {
        EdgeCollection.insert( EdgeCollection.end(), this->Vertices [ i ]->Edges.begin(), this->Vertices [ i ]->Edges.end() );
    }

    // Sort and check if neighbours in vector are copies. If so, we have found an edge of the triangle.
    std :: sort( EdgeCollection.begin(), EdgeCollection.end() );
    std :: array< EdgeType *, 3 > DupEdges;
    int i=0;
    size_t j=0;
    while (j<EdgeCollection.size()) {
        if ( (j+1)==EdgeCollection.size() ) {
            break;
        }
        if (EdgeCollection.at(j+1)==EdgeCollection.at(j)) {
            DupEdges.at(i) = EdgeCollection.at(j);
            i++;
            j++;
        }
        j++;
    }

    return DupEdges;

}

double TriangleType :: GiveLongestEdgeLength()
{
    double l = 0.0;
    for ( int i = 0; i < 3; i++ ) {
        int nexti = ( i == 2 ) ? 0 : i + 1;
        double d0 = ( this->Vertices [ i ]->get_c(0) - this->Vertices [ nexti ]->get_c(0) );
        double d1 = ( this->Vertices [ i ]->get_c(1) - this->Vertices [ nexti ]->get_c(1) );
        double d2 = ( this->Vertices [ i ]->get_c(2) - this->Vertices [ nexti ]->get_c(2) );
        double alpha = sqrt(d0 * d0 + d1 * d1 + d2 * d2);
        if ( alpha > l ) {
            l = alpha;
        }
    }
    return l;
}

void TriangleType :: UpdateNormal()
{
    std :: array< double, 3 >edge0 = this->GiveEdgeVector(0);
    std :: array< double, 3 >edge1 = this->GiveEdgeVector(1);

    this->Normal [ 0 ] = edge0 [ 1 ] * edge1 [ 2 ] - edge1 [ 1 ] * edge0 [ 2 ];
    this->Normal [ 1 ] = -edge0 [ 0 ] * edge1 [ 2 ] + edge1 [ 0 ] * edge0 [ 2 ];
    this->Normal [ 2 ] = edge0 [ 0 ] * edge1 [ 1 ] - edge1 [ 0 ] * edge0 [ 1 ];
}

void TriangleType :: FlipNormal()
{
    VertexType *v = this->Vertices [ 0 ];
    this->Vertices [ 0 ] = this->Vertices [ 1 ];
    this->Vertices [ 1 ] = v;

    int PosPhase = this->PosNormalMatID;
    this->PosNormalMatID = this->NegNormalMatID;
    this->NegNormalMatID = PosPhase;

    this->UpdateNormal();
}

// TetType

std :: array< double, 3 >TetType :: GiveCenterOfMass()
{
    std :: array< double, 3 >cm = { { 0, 0, 0 } };
    for ( VertexType *v : this->Vertices ) {
        for ( int i = 0; i < 3; i++ ) {
            cm [ i ] = cm [ i ] + v->get_c(i) / 4;
        }
    }
    return cm;
}
}
