#include "OOFEMExporter.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <vector>

namespace voxel2tet
{

OOFEMExporter :: OOFEMExporter(std :: vector< TriangleType * > *Triangles, std :: vector< VertexType * > *Vertices, std :: vector< EdgeType * > *Edges, std :: vector< TetType * > *Tets) :
    Exporter(Triangles, Vertices, Edges, Tets)
{
    LOG("Create OOFEMExporter object\n", 0);
}


void OOFEMExporter :: WriteVolumeData(std :: string Filename)
{
    STATUS( "Write .in (oofem) file %s\n", Filename.c_str() );
    std :: ofstream OOFEMFile;
    OOFEMFile.open(Filename);

    // Prepare information

    // Used vertices
    this->UpdateUsedVertices();

    // Materials
    this->UpdateMaterialsMapping();

    // Find node sets
    this->UpdateMinMaxCoordinates();

    // Max and min nodes are arrays of lists of vertices where the index of the array tells in which direction the vertex is located
    this->UpdateMinMaxNodes();

    // Find element boundaries
    this->UpdateMinMaxElements();

    // Write header

    OOFEMFile << Filename << ".out\n";
    OOFEMFile << "Exported from Voxel2Tet\n";
    OOFEMFile << "staticstructural nsteps 1 deltaT 1 lstype 3 smtype 7 rtolf 1.e-8 nmodules 1 maxiter 50\n";
    OOFEMFile << "vtkxml tstep_all domain_all domain_all primvars 1 1 cellvars 1 1 \n";
    OOFEMFile << "domain 3d\n";
    OOFEMFile << "OutputManager tstep_all dofman_all element_all\n";
    OOFEMFile << "ndofman " << UsedVertices.size() << " nelem " << this->Tets->size() << " ncrosssect 1 nmat " \
              << Self2OofemMaterials.size() << " nbc 1 nic 1 nltf 1 nset 15 nxfemman 0\n";

    // Write vertices

    for ( size_t i = 0; i < this->Vertices->size(); i++ ) {
        OOFEMFile << "node " << i + 1 << "\tcoords 3 " << UsedVertices.at(i)->get_c(0) << "\t" \
                  << UsedVertices.at(i)->get_c(1) << "\t" << UsedVertices.at(i)->get_c(2) << "\n";
    }

    // Write elements

    for ( size_t i = 0; i < this->Tets->size(); i++ ) {
        TetType *t = this->Tets->at(i);
        OOFEMFile << "ltrspace " << i + 1 << "\tnodes 4\t" << t->Vertices [ 0 ]->tag + 1 \
                  << "\t" << t->Vertices [ 1 ]->tag + 1 << "\t" << t->Vertices [ 2 ]->tag + 1 << "\t" << t->Vertices [ 3 ]->tag + 1 \
                  << "\tcrosssect 1 \tmat " << Self2OofemMaterials [ t->MaterialID ] << "\n";
    }

    // Write cross-section
    OOFEMFile << "SimpleCS 1 thick 0.1 width 1.0\n";

    // Write Materials
    int i = 1;
    for ( auto test : Self2OofemMaterials ) {
        OOFEMFile << "# Material " << test.first << " in source file\n";
        //OOFEMFile << "hyperelmat " << i++ << " d 1 k " << 100 + i*10 << " g " << 100 + i*10 << "\n";
        OOFEMFile << "IsoLE " << i << " d 1.0 E " << 200 + i * 10 << "e9 n 0.3 tAlpha 0.0\n";
        i++;
        // OOFEMFile << "isole " << i++ << " d 1 E 1 n .5 tAlpha 0\n";
    }

    // Write boundary conditions
    std :: array< double, 3 >center = { { ( MaxCoords [ 0 ] + MinCoords [ 0 ] ) / 2.0, ( MaxCoords [ 1 ] + MinCoords [ 1 ] ) / 2.0, ( MaxCoords [ 2 ] + MinCoords [ 2 ] ) / 2.0 } };
    OOFEMFile << "PrescribedGradient 1 loadTimeFunction 1 ccoord 3 " << center [ 0 ] << " " \
              << center [ 1 ] << " " << center [ 2 ] << " gradient 3 3 {0.2 0.0 0.0;0.0 0.0 0.0;0.0 0.0 0.0} set 2 dofs 3 1 2 3\n";

    // Write initial condition and load function
    OOFEMFile << "InitialCondition 1 Conditions 0 set 0\n";
    OOFEMFile << "ConstantFunction 1 f(t) 1.0\n";


    // Write node sets
    int setid = 1;

    // Volume set
    OOFEMFile << "# Volume set\n";
    OOFEMFile << "set " << setid++ << " elements " << this->Tets->size();
    for ( size_t i = 0; i < this->Tets->size(); i++ ) {
        OOFEMFile << " " << i + 1;
    }
    OOFEMFile << "\n";

    // Boundary set
    OOFEMFile << "# Complete node boundary set\n";
    OOFEMFile << "set " << setid++ << " nodes " << ( MaxNodes [ 0 ].size() + MaxNodes [ 1 ].size() + MaxNodes [ 2 ].size() + MinNodes [ 0 ].size() + MinNodes [ 1 ].size() + MinNodes [ 2 ].size() );
    for ( int i = 0; i < 3; i++ ) {
        // Write max nodes
        for ( size_t j = 0; j < MaxNodes [ i ].size(); j++ ) {
            OOFEMFile << " " << MaxNodes [ i ].at(j)->tag + 1;
        }
        // Write min nodes
        for ( size_t j = 0; j < MinNodes [ i ].size(); j++ ) {
            OOFEMFile << " " << MinNodes [ i ].at(j)->tag + 1;
        }
    }
    OOFEMFile << "\n";

    // Individual max and min sets in all directions
    std :: array< std :: string, 3 >MaxComments = { { "# Max X", "# Max Y", "# Max Z" } };
    std :: array< std :: string, 3 >MinComments = { { "# Min X", "# Min Y", "# Min Z" } };
    for ( int i = 0; i < 3; i++ ) {
        // Write max nodes
        OOFEMFile << MaxComments [ i ] << " nodes\n";
        OOFEMFile << "set " << setid++ << " nodes " << MaxNodes [ i ].size();
        for ( size_t j = 0; j < MaxNodes [ i ].size(); j++ ) {
            OOFEMFile << " " << MaxNodes [ i ].at(j)->tag + 1;
        }
        OOFEMFile << "\n";

        // Write min nodes
        OOFEMFile << MinComments [ i ] << " nodes\n";
        OOFEMFile << "set " << setid++ << " nodes " << MinNodes [ i ].size();
        for ( size_t j = 0; j < MinNodes [ i ].size(); j++ ) {
            OOFEMFile << " " << MinNodes [ i ].at(j)->tag + 1;
        }
        OOFEMFile << "\n";
    }

    // Element boundaries

    // Complete boundary by element sides
    int ecount = 0;
    for ( std :: vector< int >s : MaxSide ) {
        ecount = ecount + s.size();
    }
    for ( std :: vector< int >s : MinSide ) {
        ecount = ecount + s.size();
    }

    OOFEMFile << "# Complete boundary by element sides\n";
    OOFEMFile << "set " << setid++ << " elementboundaries " << ecount * 2;
    for ( int k = 0; k < 2; k++ ) {
        std :: array< std :: vector< TetType * >, 3 > *Elements = ( k == 0 ) ? & MaxElements : & MinElements;
        std :: array< std :: vector< int >, 3 > *Sides = ( k == 0 ) ? & MaxSide : & MinSide;
        for ( int i = 0; i < 3; i++ ) {
            for ( size_t j = 0; j < Elements->at(i).size(); j++ ) {
                OOFEMFile << " " << Elements->at(i).at(j)->ID + 1 << " " << Sides->at(i).at(j);
            }
        }
    }
    OOFEMFile << "\n";

    // Boundaries in all directions
    for ( int k = 0; k < 2; k++ ) {
        std :: array< std :: string, 3 > *Comments = ( k == 0 ) ? & MaxComments : & MinComments;
        std :: array< std :: vector< TetType * >, 3 > *Elements = ( k == 0 ) ? & MaxElements : & MinElements;
        std :: array< std :: vector< int >, 3 > *Sides = ( k == 0 ) ? & MaxSide : & MinSide;

        for ( int i = 0; i < 3; i++ ) {
            OOFEMFile << Comments->at(i) << " element boundaries\n";
            OOFEMFile << "set " << setid++ << " elementboundaries " << Elements->at(i).size() + Sides->at(i).size();
            for ( size_t j = 0; j < Elements->at(i).size(); j++ ) {
                OOFEMFile << " " << Elements->at(i).at(j)->ID + 1 << " " << Sides->at(i).at(j);
            }
            OOFEMFile << "\n";
        }
    }
}
}
