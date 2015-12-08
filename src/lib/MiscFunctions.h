#ifndef MISCFUNCTIONS_H_
#define MISCFUNCTIONS_H_

#define LOGOUTPUT 0
#define STATOUTPUT 1

#define EXPORT_SMOOTHING_ANIMATION 0

#define LOG(format, args...) dolog (__FUNCTION__, format, args)
#define STATUS(format, args...) dooutputstat (format, args)

#define EPS 1.0e-8

#include <vector>
#include <array>
#include <string>
#include "MeshComponents.h"


namespace voxel2tet
{

class MeshData;
extern "C" int outputindex;
extern "C" MeshData* GlobalMesh;

void dolog(const char *functionname, const char *fmt, ...);
void dooutputstat(const char *fmt, ...);

void SpringSmooth(std::vector<VertexType*> Vertices, std::vector<std::array<bool,3>> FixedDirections,
                  std::vector<std::vector<VertexType*>> Connections, double K, MeshData *Mesh=NULL);

template <typename T> std::vector<int> FindSubsetIndices(std::vector<T> Container, std::vector<T> Subset);

}

#endif /* MISCFUNCTIONS_H_ */
