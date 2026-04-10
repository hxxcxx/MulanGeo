#pragma once
#include <string>
#include <vector>
#include "../Core/src/Mesh.cpp"

namespace MulanGeo {
namespace IO {

enum class CADFormat {
    Unknown,
    STEP,
    IGES,
    OBJ,
    DWF
};

class CADImporter {
public:
    CADImporter();
    ~CADImporter();

    std::vector<Core::Mesh> import(const std::string& filePath);

    CADFormat detectFormat(const std::string& filePath);

private:
    std::vector<Core::Mesh> importStep(const std::string& filePath);
    std::vector<Core::Mesh> importObj(const std::string& filePath);
    std::vector<Core::Mesh> importDwf(const std::string& filePath);

#ifdef HAVE_OCC
    void* m_occDoc = nullptr;
#endif
};

}
}
