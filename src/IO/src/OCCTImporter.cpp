#include "MulanGeo/IO/OCCTImporter.h"
#include "MulanGeo/IO/MeshData.h"
#include "MulanGeo/IO/ImporterFactory.h"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Builder.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <TopLoc_Location.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <Interface_Static.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_Path.hxx>

#include <algorithm>
#include <clocale>
#include <filesystem>

namespace MulanGeo::IO {

namespace {

double computeLinearDeflection(const TopoDS_Shape& shape) {
    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    if (box.IsVoid()) return 0.1;

    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    double dx = xmax - xmin;
    double dy = ymax - ymin;
    double dz = zmax - zmin;
    double maxDim = std::max({dx, dy, dz});

    return maxDim * 0.001;  // 0.1% of max dimension
}

TopoDS_Shape readSTEP(const std::string& path) {
    STEPControl_Reader reader;
    Interface_Static::SetIVal("read.stepbspline.continuity", 2);

    IFSelect_ReturnStatus status = reader.ReadFile(path.c_str());
    if (status != IFSelect_RetDone) {
        throw std::runtime_error("Failed to read STEP file: " + path);
    }

    reader.TransferRoots();
    TopoDS_Shape shape = reader.OneShape();
    if (shape.IsNull()) {
        throw std::runtime_error("No valid shape found in STEP file");
    }
    return shape;
}

TopoDS_Shape readIGES(const std::string& path) {
    IGESControl_Reader reader;
    IFSelect_ReturnStatus status = reader.ReadFile(path.c_str());
    if (status != IFSelect_RetDone) {
        throw std::runtime_error("Failed to read IGES file: " + path);
    }

    reader.TransferRoots();
    TopoDS_Shape shape = reader.OneShape();
    if (shape.IsNull()) {
        throw std::runtime_error("No valid shape found in IGES file");
    }
    return shape;
}

TopoDS_Shape readFile(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (ext == ".step" || ext == ".stp") return readSTEP(path);
    if (ext == ".iges" || ext == ".igs") return readIGES(path);

    throw std::runtime_error("Unsupported format: " + ext);
}

void triangulate(TopoDS_Shape& shape, double linearDeflection) {
    BRepMesh_IncrementalMesh mesher(shape, linearDeflection, false, 0.5, true);
    mesher.Perform();
    if (!mesher.IsDone()) {
        throw std::runtime_error("Mesh generation failed");
    }
}

MeshPart extractMesh(const TopoDS_Shape& shape) {
    MeshPart meshPart;

    for (TopExp_Explorer faceExp(shape, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
        TopoDS_Face face = TopoDS::Face(faceExp.Current());
        TopLoc_Location loc;

        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) continue;

        const gp_Trsf& trsf = loc.Transformation();
        int baseIndex = static_cast<int>(meshPart.vertices.size());
        int nbNodes = tri->NbNodes();

        for (int i = 1; i <= nbNodes; i++) {
            gp_Pnt p = tri->Node(i).Transformed(trsf);
            gp_Dir n(0, 0, 1);
            if (tri->HasNormals()) {
                n = tri->Normal(i).Transformed(trsf);
            }
            meshPart.vertices.push_back({
                {static_cast<float>(p.X()), static_cast<float>(p.Y()), static_cast<float>(p.Z())},
                {static_cast<float>(n.X()), static_cast<float>(n.Y()), static_cast<float>(n.Z())},
                {0, 0}
            });
        }

        int nbTris = tri->NbTriangles();
        for (int i = 1; i <= nbTris; i++) {
            int n0, n1, n2;
            tri->Triangle(i).Get(n0, n1, n2);
            meshPart.indices.push_back(static_cast<uint32_t>(baseIndex + n0 - 1));
            meshPart.indices.push_back(static_cast<uint32_t>(baseIndex + n1 - 1));
            meshPart.indices.push_back(static_cast<uint32_t>(baseIndex + n2 - 1));
        }
    }

    return meshPart;
}

} // anonymous namespace

ImportResult OCCTImporter::importFile(const std::string& path) {
    ImportResult result;
    result.sourceFile = path;

    try {
        // OCCT needs C locale for file parsing
        auto* oldLocale = std::setlocale(LC_NUMERIC, "C");

        TopoDS_Shape shape = readFile(path);

        if (shape.ShapeType() == TopAbs_COMPOUND || shape.ShapeType() == TopAbs_COMPSOLID) {
            // Try to split into separate solids
            bool hasSolids = false;
            for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) {
                hasSolids = true;
                TopoDS_Shape solid = exp.Current();
                double deflection = computeLinearDeflection(solid);
                triangulate(solid, deflection);
                auto part = extractMesh(solid);
                if (!part.vertices.empty()) {
                    result.meshes.push_back(std::move(part));
                }
            }
            if (!hasSolids) {
                // Fallback: mesh entire shape as one
                double deflection = computeLinearDeflection(shape);
                triangulate(shape, deflection);
                auto part = extractMesh(shape);
                if (!part.vertices.empty()) {
                    result.meshes.push_back(std::move(part));
                }
            }
        } else {
            double deflection = computeLinearDeflection(shape);
            triangulate(shape, deflection);
            auto part = extractMesh(shape);
            if (!part.vertices.empty()) {
                result.meshes.push_back(std::move(part));
            }
        }

        // Restore locale
        if (oldLocale) std::setlocale(LC_NUMERIC, oldLocale);

        result.success = true;
    } catch (const std::exception& e) {
        result.error = e.what();
        result.success = false;
    }

    return result;
}

std::vector<std::string> OCCTImporter::supportedExtensions() const {
    return {"step", "stp", "iges", "igs"};
}

std::string OCCTImporter::name() const {
    return "OCCT Importer";
}

static AutoRegisterImporter _reg_step("step", []() -> std::unique_ptr<IFileImporter> {
    return std::make_unique<OCCTImporter>();
});
static AutoRegisterImporter _reg_stp("stp", []() -> std::unique_ptr<IFileImporter> {
    return std::make_unique<OCCTImporter>();
});
static AutoRegisterImporter _reg_iges("iges", []() -> std::unique_ptr<IFileImporter> {
    return std::make_unique<OCCTImporter>();
});
static AutoRegisterImporter _reg_igs("igs", []() -> std::unique_ptr<IFileImporter> {
    return std::make_unique<OCCTImporter>();
});

} // namespace MulanGeo::IO
