#include <vector>
#include "MeshData.h"
#include "MiscFunctions.h"
#include "TetGenExporter.h"
#include "OFFExporter.h"

namespace voxel2tet
{


MeshData::MeshData(BoundingBoxType BoundingBox)
{
    this->BoundingBox = BoundingBox;
    this->VertexOctreeRoot = new VertexOctreeNode(this->BoundingBox, &this->Vertices, 0);
    this->TriangleCounter = 0;
}

MeshData::~MeshData()
{
    for (unsigned int i=0; i<this->Edges.size(); i++) {
        delete this->Edges.at(i);
    }

    for (auto t: this->Triangles) {
        delete t;
    }

    for (auto v: this->Vertices) delete v;

    delete this->VertexOctreeRoot;
}

void MeshData :: DoSanityCheck()
{
    // Does the list of triangles vertices correspond to the list of vertices triangles?
    for (TriangleType *t: this->Triangles) {
        bool TriangleFound = false;
        for (VertexType *v: t->Vertices) {
            for (TriangleType *vt: v->Triangles) {
                if (vt==t) {
                    TriangleFound = true;
                }
            }
        }
        if (!TriangleFound) {
            STATUS ("Triangles and vertices does not match!\n", 0);
            throw 0;
        }
        if (t->ID>this->TriangleCounter) {
            STATUS ("Invalid triangle ID!\n", 0);
            throw 0;
        }
    }

    // Does the list of edge vertices correspond to the list of vertices triangles?
    for (EdgeType *e: this->Edges) {
        bool EdgeFound = false;
        for (VertexType *v: e->Vertices) {
            for (EdgeType *ve: v->Edges) {
                if (ve==e) {
                    EdgeFound = true;
                }
            }
        }
        if (!EdgeFound) {
            STATUS ("Edges and vertices does not match!\n", 0);
            throw 0;
        }
    }

    // Check the same, but for vertices
    for (VertexType *v: this->Vertices) {
        bool VertexFoundInTriangle = (v->Triangles.size()==0) ? true : false;

        for (TriangleType *t: v->Triangles) {

            for (VertexType *vt: t->Vertices) {
                if (vt==v) {
                    VertexFoundInTriangle = true;
                }
            }
        }

        if (!VertexFoundInTriangle) {
            STATUS ("Triangle does not contain vertex while vertex contains triangle\n", 0);
            throw 0;
        }

        bool VertexFoundInEdge = (v->Edges.size()==0) ? true : false;

        for (EdgeType *e: v->Edges) {
            for (VertexType *ve: e->Vertices) {
                if (ve==v) {
                    VertexFoundInEdge = true;
                }
            }
        }

        if (!VertexFoundInEdge) {
            STATUS("Vertex not found in edge list while edge is found in vertex list of edges\n", 0);
            throw 0;
        }

    }


    // Are all triangles unique?
    for (TriangleType *t1: this->Triangles) {
        for (TriangleType *t2: this->Triangles) {
            if (t1!=t2) {
                bool ispermutation = std::is_permutation(t1->Vertices.begin(), t1->Vertices.end(), t2->Vertices.begin());
                if (ispermutation) {
                    STATUS ("\nDuplicate triangles\n", 0);
                    throw 0;
                }
            }
        }
    }
}

void MeshData :: ExportSurface(std::string FileName, Exporter_FileTypes FileType)
{
    STATUS ("Export to %s\n", FileName.c_str());
    Exporter *exporter;
    switch (FileType) {
    case FT_OFF: {
        exporter = new OFFExporter (&this->Triangles, &this->Vertices, &this->Edges);
        break;
    }
    case FT_Poly: {
        exporter = new TetGenExporter (&this->Triangles, &this->Vertices, &this->Edges);
        break;
    }
    case FT_VTK: {
        exporter = new VTKExporter(&this->Triangles, &this->Vertices, &this->Edges);
        break;
    }
    }
    exporter->WriteData(FileName);
    free(exporter);
}

EdgeType *MeshData :: AddEdge(std::vector<int> VertexIDs)
{

    // Check if edge already exists
    int ThisVertexID = VertexIDs.at(0);
    int OtherVertexID = VertexIDs.at(1);
    VertexType *ThisVertex = this->Vertices.at(ThisVertexID);
    VertexType *OtherVertex = this->Vertices.at(OtherVertexID);

    std::vector <EdgeType*> Edges = this->Vertices.at(ThisVertexID)->Edges;
    for (auto Edge: Edges) {
        if ( ( (Edge->Vertices[0] == ThisVertex) & (Edge->Vertices[1] == OtherVertex) ) | ( (Edge->Vertices[1] == ThisVertex) & (Edge->Vertices[0] == OtherVertex) )) {
            LOG("Edge %p already exists\n", Edge);
            return Edge;
        }
    }

    EdgeType *NewEdge = new EdgeType;
    LOG("Create edge from Vertex IDs %u and %u: %p\n", VertexIDs.at(0), VertexIDs.at(1), NewEdge);
    for (unsigned int i: {0, 1}) {
        NewEdge->Vertices.at(i) =  this->Vertices.at(VertexIDs.at(i));
    }

    return AddEdge(NewEdge);
}

EdgeType *MeshData :: AddEdge(EdgeType *e) {
    for (VertexType *v: e->Vertices) {
        v->AddEdge(e);
    }
    this->Edges.push_back(e);
    return e;
}

void MeshData :: RemoveEdge(EdgeType *e)
{
    for (VertexType *v: e->Vertices) {
        v->RemoveEdge(e);
    }
    this->Edges.erase(std::remove(this->Edges.begin(), this->Edges.end(), e), this->Edges.end());
    delete e;
}

void MeshData :: RemoveTriangle(TriangleType *t)
{
    LOG("Remove triangle %u\n", t->ID);
    for (VertexType *v: t->Vertices) {
        v->RemoveTriangle(t);
    }
    this->Triangles.erase(std::remove(this->Triangles.begin(), this->Triangles.end(), t), this->Triangles.end());
    delete t;
}

TriangleType *MeshData :: AddTriangle(std::vector<double> n0, std::vector<double> n1, std::vector<double> n2)
{
    // Insert vertices and create a triangle using the indices returned
    int VertexIDs[3];

    VertexIDs[0] = this->VertexOctreeRoot->AddVertex(n0[0], n0[1], n0[2]);
    VertexIDs[1] = this->VertexOctreeRoot->AddVertex(n1[0], n1[1], n1[2]);
    VertexIDs[2] = this->VertexOctreeRoot->AddVertex(n2[0], n2[1], n2[2]);

    return this->AddTriangle({VertexIDs[0], VertexIDs[1], VertexIDs[2]});
}

TriangleType *MeshData :: AddTriangle(std::vector<int> VertexIDs)
{

    TriangleType *NewTriangle = new TriangleType;
    LOG ("Create triangle %p from vertices (%u, %u, %u)@(%p, %p, %p)\n", NewTriangle, VertexIDs.at(0), VertexIDs.at(1), VertexIDs.at(2),
         this->Vertices.at(VertexIDs.at(0)), this->Vertices.at(VertexIDs.at(1)), this->Vertices.at(VertexIDs.at(2)));

    for (int i=0; i<3; i++) {
        if (i<2) {
            this->AddEdge({VertexIDs.at(i), VertexIDs.at(i+1)});
        } else {
            this->AddEdge({VertexIDs.at(i), VertexIDs.at(0)});
        }
    }

    // Update vertices and triangles with references to each other
    for (int i=0; i<3; i++) {
        this->Vertices.at(VertexIDs.at(i))->AddTriangle(NewTriangle);
        NewTriangle->Vertices[i]=this->Vertices.at(VertexIDs.at(i));
    }

    return AddTriangle(NewTriangle);

}

TriangleType *MeshData :: AddTriangle(TriangleType *NewTriangle)
{
#if SANITYCHECK == 1
    int i=0;
    for (TriangleType *t: this->Triangles) {
        bool permutation = std::is_permutation(t->Vertices.begin(), t->Vertices.end(), NewTriangle->Vertices.begin());
        if (permutation) {
            STATUS("\nTriangle already exist. Existing ID = %u (index %u in list)!\n", t->ID, i); //TODO: Add a logging command for errors
            return t;
            throw 0;
        }
        i++;
    }
#endif

    NewTriangle->UpdateNormal();
    NewTriangle->ID = TriangleCounter;
    LOG("Add triangle %u to set\n", NewTriangle->ID);
    for (VertexType *v: NewTriangle->Vertices) {
        v->AddTriangle(NewTriangle);
    }
    TriangleCounter++;
    this->Triangles.push_back(NewTriangle);
    return NewTriangle;

}

}